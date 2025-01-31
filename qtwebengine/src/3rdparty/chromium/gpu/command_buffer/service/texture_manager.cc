// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/texture_manager.h"
#include "base/bits.h"
#include "base/strings/stringprintf.h"
#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "gpu/command_buffer/service/context_state.h"
#include "gpu/command_buffer/service/error_state.h"
#include "gpu/command_buffer/service/feature_info.h"
#include "gpu/command_buffer/service/framebuffer_manager.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "gpu/command_buffer/service/memory_tracking.h"

namespace gpu {
namespace gles2 {

static size_t GLTargetToFaceIndex(GLenum target) {
  switch (target) {
    case GL_TEXTURE_2D:
    case GL_TEXTURE_EXTERNAL_OES:
    case GL_TEXTURE_RECTANGLE_ARB:
      return 0;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      return 0;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      return 1;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      return 2;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      return 3;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      return 4;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      return 5;
    default:
      NOTREACHED();
      return 0;
  }
}

static size_t FaceIndexToGLTarget(size_t index) {
  switch (index) {
    case 0:
      return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    case 1:
      return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
    case 2:
      return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
    case 3:
      return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
    case 4:
      return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
    case 5:
      return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
    default:
      NOTREACHED();
      return 0;
  }
}

TextureManager::DestructionObserver::DestructionObserver() {}

TextureManager::DestructionObserver::~DestructionObserver() {}

TextureManager::~TextureManager() {
  for (unsigned int i = 0; i < destruction_observers_.size(); i++)
    destruction_observers_[i]->OnTextureManagerDestroying(this);

  DCHECK(textures_.empty());

  // If this triggers, that means something is keeping a reference to
  // a Texture belonging to this.
  CHECK_EQ(texture_count_, 0u);

  DCHECK_EQ(0, num_unrenderable_textures_);
  DCHECK_EQ(0, num_unsafe_textures_);
  DCHECK_EQ(0, num_uncleared_mips_);
  DCHECK_EQ(0, num_images_);
}

void TextureManager::Destroy(bool have_context) {
  have_context_ = have_context;
  textures_.clear();
  for (int ii = 0; ii < kNumDefaultTextures; ++ii) {
    default_textures_[ii] = NULL;
  }

  if (have_context) {
    glDeleteTextures(arraysize(black_texture_ids_), black_texture_ids_);
  }

  DCHECK_EQ(0u, memory_tracker_managed_->GetMemRepresented());
  DCHECK_EQ(0u, memory_tracker_unmanaged_->GetMemRepresented());
}

Texture::Texture(GLuint service_id)
    : mailbox_manager_(NULL),
      memory_tracking_ref_(NULL),
      service_id_(service_id),
      cleared_(true),
      num_uncleared_mips_(0),
      target_(0),
      min_filter_(GL_NEAREST_MIPMAP_LINEAR),
      mag_filter_(GL_LINEAR),
      wrap_s_(GL_REPEAT),
      wrap_t_(GL_REPEAT),
      usage_(GL_NONE),
      pool_(GL_TEXTURE_POOL_UNMANAGED_CHROMIUM),
      max_level_set_(-1),
      texture_complete_(false),
      cube_complete_(false),
      npot_(false),
      has_been_bound_(false),
      framebuffer_attachment_count_(0),
      immutable_(false),
      has_images_(false),
      estimated_size_(0),
      can_render_condition_(CAN_RENDER_ALWAYS),
      texture_max_anisotropy_initialized_(false) {
}

Texture::~Texture() {
  if (mailbox_manager_)
    mailbox_manager_->TextureDeleted(this);
}

void Texture::AddTextureRef(TextureRef* ref) {
  DCHECK(refs_.find(ref) == refs_.end());
  refs_.insert(ref);
  if (!memory_tracking_ref_) {
    memory_tracking_ref_ = ref;
    GetMemTracker()->TrackMemAlloc(estimated_size());
  }
}

void Texture::RemoveTextureRef(TextureRef* ref, bool have_context) {
  if (memory_tracking_ref_ == ref) {
    GetMemTracker()->TrackMemFree(estimated_size());
    memory_tracking_ref_ = NULL;
  }
  size_t result = refs_.erase(ref);
  DCHECK_EQ(result, 1u);
  if (refs_.empty()) {
    if (have_context) {
      GLuint id = service_id();
      glDeleteTextures(1, &id);
    }
    delete this;
  } else if (memory_tracking_ref_ == NULL) {
    // TODO(piman): tune ownership semantics for cross-context group shared
    // textures.
    memory_tracking_ref_ = *refs_.begin();
    GetMemTracker()->TrackMemAlloc(estimated_size());
  }
}

MemoryTypeTracker* Texture::GetMemTracker() {
  DCHECK(memory_tracking_ref_);
  return memory_tracking_ref_->manager()->GetMemTracker(pool_);
}

Texture::LevelInfo::LevelInfo()
    : cleared(true),
      target(0),
      level(-1),
      internal_format(0),
      width(0),
      height(0),
      depth(0),
      border(0),
      format(0),
      type(0),
      estimated_size(0) {
}

Texture::LevelInfo::LevelInfo(const LevelInfo& rhs)
    : cleared(rhs.cleared),
      target(rhs.target),
      level(rhs.level),
      internal_format(rhs.internal_format),
      width(rhs.width),
      height(rhs.height),
      depth(rhs.depth),
      border(rhs.border),
      format(rhs.format),
      type(rhs.type),
      image(rhs.image),
      estimated_size(rhs.estimated_size) {
}

Texture::LevelInfo::~LevelInfo() {
}

Texture::CanRenderCondition Texture::GetCanRenderCondition() const {
  if (target_ == 0)
    return CAN_RENDER_ALWAYS;

  if (target_ != GL_TEXTURE_EXTERNAL_OES) {
    if (level_infos_.empty()) {
      return CAN_RENDER_NEVER;
    }

    const Texture::LevelInfo& first_face = level_infos_[0][0];
    if (first_face.width == 0 ||
        first_face.height == 0 ||
        first_face.depth == 0) {
      return CAN_RENDER_NEVER;
    }
  }

  bool needs_mips = NeedsMips();
  if (needs_mips) {
    if (!texture_complete())
      return CAN_RENDER_NEVER;
    if (target_ == GL_TEXTURE_CUBE_MAP && !cube_complete())
      return CAN_RENDER_NEVER;
  }

  bool is_npot_compatible = !needs_mips &&
      wrap_s_ == GL_CLAMP_TO_EDGE &&
      wrap_t_ == GL_CLAMP_TO_EDGE;

  if (!is_npot_compatible) {
    if (target_ == GL_TEXTURE_RECTANGLE_ARB)
      return CAN_RENDER_NEVER;
    else if (npot())
      return CAN_RENDER_ONLY_IF_NPOT;
  }

  return CAN_RENDER_ALWAYS;
}

bool Texture::CanRender(const FeatureInfo* feature_info) const {
  switch (can_render_condition_) {
    case CAN_RENDER_ALWAYS:
      return true;
    case CAN_RENDER_NEVER:
      return false;
    case CAN_RENDER_ONLY_IF_NPOT:
      break;
  }
  return feature_info->feature_flags().npot_ok;
}

void Texture::AddToSignature(
    const FeatureInfo* feature_info,
    GLenum target,
    GLint level,
    std::string* signature) const {
  DCHECK(feature_info);
  DCHECK(signature);
  DCHECK_GE(level, 0);
  DCHECK_LT(static_cast<size_t>(GLTargetToFaceIndex(target)),
            level_infos_.size());
  DCHECK_LT(static_cast<size_t>(level),
            level_infos_[GLTargetToFaceIndex(target)].size());
  const Texture::LevelInfo& info =
      level_infos_[GLTargetToFaceIndex(target)][level];
  *signature += base::StringPrintf(
      "|Texture|target=%04x|level=%d|internal_format=%04x"
      "|width=%d|height=%d|depth=%d|border=%d|format=%04x|type=%04x"
      "|image=%d|canrender=%d|canrenderto=%d|npot_=%d"
      "|min_filter=%04x|mag_filter=%04x|wrap_s=%04x|wrap_t=%04x"
      "|usage=%04x",
      target, level, info.internal_format,
      info.width, info.height, info.depth, info.border,
      info.format, info.type, info.image.get() != NULL,
      CanRender(feature_info), CanRenderTo(), npot_,
      min_filter_, mag_filter_, wrap_s_, wrap_t_,
      usage_);
}

void Texture::SetMailboxManager(MailboxManager* mailbox_manager) {
  DCHECK(!mailbox_manager_ || mailbox_manager_ == mailbox_manager);
  mailbox_manager_ = mailbox_manager;
}

bool Texture::MarkMipmapsGenerated(
    const FeatureInfo* feature_info) {
  if (!CanGenerateMipmaps(feature_info)) {
    return false;
  }
  for (size_t ii = 0; ii < level_infos_.size(); ++ii) {
    const Texture::LevelInfo& info1 = level_infos_[ii][0];
    GLsizei width = info1.width;
    GLsizei height = info1.height;
    GLsizei depth = info1.depth;
    GLenum target = target_ == GL_TEXTURE_2D ? GL_TEXTURE_2D :
                               FaceIndexToGLTarget(ii);
    int num_mips =
        TextureManager::ComputeMipMapCount(target_, width, height, depth);
    for (int level = 1; level < num_mips; ++level) {
      width = std::max(1, width >> 1);
      height = std::max(1, height >> 1);
      depth = std::max(1, depth >> 1);
      SetLevelInfo(feature_info,
                   target,
                   level,
                   info1.internal_format,
                   width,
                   height,
                   depth,
                   info1.border,
                   info1.format,
                   info1.type,
                   true);
    }
  }

  return true;
}

void Texture::SetTarget(
    const FeatureInfo* feature_info, GLenum target, GLint max_levels) {
  DCHECK_EQ(0u, target_);  // you can only set this once.
  target_ = target;
  size_t num_faces = (target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
  level_infos_.resize(num_faces);
  for (size_t ii = 0; ii < num_faces; ++ii) {
    level_infos_[ii].resize(max_levels);
  }

  if (target == GL_TEXTURE_EXTERNAL_OES || target == GL_TEXTURE_RECTANGLE_ARB) {
    min_filter_ = GL_LINEAR;
    wrap_s_ = wrap_t_ = GL_CLAMP_TO_EDGE;
  }

  if (target == GL_TEXTURE_EXTERNAL_OES) {
    immutable_ = true;
  }
  Update(feature_info);
  UpdateCanRenderCondition();
}

bool Texture::CanGenerateMipmaps(
    const FeatureInfo* feature_info) const {
  if ((npot() && !feature_info->feature_flags().npot_ok) ||
      level_infos_.empty() ||
      target_ == GL_TEXTURE_EXTERNAL_OES ||
      target_ == GL_TEXTURE_RECTANGLE_ARB) {
    return false;
  }

  // Can't generate mips for depth or stencil textures.
  const Texture::LevelInfo& first = level_infos_[0][0];
  uint32 channels = GLES2Util::GetChannelsForFormat(first.format);
  if (channels & (GLES2Util::kDepth | GLES2Util::kStencil)) {
    return false;
  }

  // TODO(gman): Check internal_format, format and type.
  for (size_t ii = 0; ii < level_infos_.size(); ++ii) {
    const LevelInfo& info = level_infos_[ii][0];
    if ((info.target == 0) || (info.width != first.width) ||
        (info.height != first.height) || (info.depth != 1) ||
        (info.format != first.format) ||
        (info.internal_format != first.internal_format) ||
        (info.type != first.type) ||
        feature_info->validators()->compressed_texture_format.IsValid(
            info.internal_format) ||
        info.image.get()) {
      return false;
    }
  }
  return true;
}

void Texture::SetLevelCleared(GLenum target, GLint level, bool cleared) {
  DCHECK_GE(level, 0);
  DCHECK_LT(static_cast<size_t>(GLTargetToFaceIndex(target)),
            level_infos_.size());
  DCHECK_LT(static_cast<size_t>(level),
            level_infos_[GLTargetToFaceIndex(target)].size());
  Texture::LevelInfo& info =
      level_infos_[GLTargetToFaceIndex(target)][level];
  UpdateMipCleared(&info, cleared);
  UpdateCleared();
}

void Texture::UpdateCleared() {
  if (level_infos_.empty()) {
    return;
  }

  const Texture::LevelInfo& first_face = level_infos_[0][0];
  int levels_needed = TextureManager::ComputeMipMapCount(
      target_, first_face.width, first_face.height, first_face.depth);
  bool cleared = true;
  for (size_t ii = 0; ii < level_infos_.size(); ++ii) {
    for (GLint jj = 0; jj < levels_needed; ++jj) {
      const Texture::LevelInfo& info = level_infos_[ii][jj];
      if (info.width > 0 && info.height > 0 && info.depth > 0 &&
          !info.cleared) {
        cleared = false;
        break;
      }
    }
  }

  // If texture is uncleared and is attached to a framebuffer,
  // that framebuffer must be marked possibly incomplete.
  if (!cleared && IsAttachedToFramebuffer()) {
    IncAllFramebufferStateChangeCount();
  }

  UpdateSafeToRenderFrom(cleared);
}

void Texture::UpdateSafeToRenderFrom(bool cleared) {
  if (cleared_ == cleared)
    return;
  cleared_ = cleared;
  int delta = cleared ? -1 : +1;
  for (RefSet::iterator it = refs_.begin(); it != refs_.end(); ++it)
    (*it)->manager()->UpdateSafeToRenderFrom(delta);
}

void Texture::UpdateMipCleared(LevelInfo* info, bool cleared) {
  if (info->cleared == cleared)
    return;
  info->cleared = cleared;
  int delta = cleared ? -1 : +1;
  num_uncleared_mips_ += delta;
  for (RefSet::iterator it = refs_.begin(); it != refs_.end(); ++it)
    (*it)->manager()->UpdateUnclearedMips(delta);
}

void Texture::UpdateCanRenderCondition() {
  CanRenderCondition can_render_condition = GetCanRenderCondition();
  if (can_render_condition_ == can_render_condition)
    return;
  for (RefSet::iterator it = refs_.begin(); it != refs_.end(); ++it)
    (*it)->manager()->UpdateCanRenderCondition(can_render_condition_,
                                               can_render_condition);
  can_render_condition_ = can_render_condition;
}

void Texture::UpdateHasImages() {
  if (level_infos_.empty())
    return;

  bool has_images = false;
  for (size_t ii = 0; ii < level_infos_.size(); ++ii) {
    for (size_t jj = 0; jj < level_infos_[ii].size(); ++jj) {
      const Texture::LevelInfo& info = level_infos_[ii][jj];
      if (info.image.get() != NULL) {
        has_images = true;
        break;
      }
    }
  }

  if (has_images_ == has_images)
    return;
  has_images_ = has_images;
  int delta = has_images ? +1 : -1;
  for (RefSet::iterator it = refs_.begin(); it != refs_.end(); ++it)
    (*it)->manager()->UpdateNumImages(delta);
}

void Texture::IncAllFramebufferStateChangeCount() {
  for (RefSet::iterator it = refs_.begin(); it != refs_.end(); ++it)
    (*it)->manager()->IncFramebufferStateChangeCount();
}

void Texture::SetLevelInfo(
    const FeatureInfo* feature_info,
    GLenum target,
    GLint level,
    GLenum internal_format,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLint border,
    GLenum format,
    GLenum type,
    bool cleared) {
  DCHECK_GE(level, 0);
  DCHECK_LT(static_cast<size_t>(GLTargetToFaceIndex(target)),
            level_infos_.size());
  DCHECK_LT(static_cast<size_t>(level),
            level_infos_[GLTargetToFaceIndex(target)].size());
  DCHECK_GE(width, 0);
  DCHECK_GE(height, 0);
  DCHECK_GE(depth, 0);
  Texture::LevelInfo& info =
      level_infos_[GLTargetToFaceIndex(target)][level];
  info.target = target;
  info.level = level;
  info.internal_format = internal_format;
  info.width = width;
  info.height = height;
  info.depth = depth;
  info.border = border;
  info.format = format;
  info.type = type;
  info.image = 0;

  estimated_size_ -= info.estimated_size;
  GLES2Util::ComputeImageDataSizes(
      width, height, format, type, 4, &info.estimated_size, NULL, NULL);
  estimated_size_ += info.estimated_size;

  UpdateMipCleared(&info, cleared);
  max_level_set_ = std::max(max_level_set_, level);
  Update(feature_info);
  UpdateCleared();
  UpdateCanRenderCondition();
  UpdateHasImages();
  if (IsAttachedToFramebuffer()) {
    // TODO(gman): If textures tracked which framebuffers they were attached to
    // we could just mark those framebuffers as not complete.
    IncAllFramebufferStateChangeCount();
  }
}

bool Texture::ValidForTexture(
    GLint target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum type) const {
  size_t face_index = GLTargetToFaceIndex(target);
  if (level >= 0 && face_index < level_infos_.size() &&
      static_cast<size_t>(level) < level_infos_[face_index].size()) {
    const LevelInfo& info = level_infos_[GLTargetToFaceIndex(target)][level];
    int32 right;
    int32 top;
    return SafeAddInt32(xoffset, width, &right) &&
           SafeAddInt32(yoffset, height, &top) &&
           xoffset >= 0 &&
           yoffset >= 0 &&
           right <= info.width &&
           top <= info.height &&
           type == info.type;
  }
  return false;
}

bool Texture::GetLevelSize(
    GLint target, GLint level, GLsizei* width, GLsizei* height) const {
  DCHECK(width);
  DCHECK(height);
  size_t face_index = GLTargetToFaceIndex(target);
  if (level >= 0 && face_index < level_infos_.size() &&
      static_cast<size_t>(level) < level_infos_[face_index].size()) {
    const LevelInfo& info = level_infos_[GLTargetToFaceIndex(target)][level];
    if (info.target != 0) {
      *width = info.width;
      *height = info.height;
      return true;
    }
  }
  return false;
}

bool Texture::GetLevelType(
    GLint target, GLint level, GLenum* type, GLenum* internal_format) const {
  DCHECK(type);
  DCHECK(internal_format);
  size_t face_index = GLTargetToFaceIndex(target);
  if (level >= 0 && face_index < level_infos_.size() &&
      static_cast<size_t>(level) < level_infos_[face_index].size()) {
    const LevelInfo& info = level_infos_[GLTargetToFaceIndex(target)][level];
    if (info.target != 0) {
      *type = info.type;
      *internal_format = info.internal_format;
      return true;
    }
  }
  return false;
}

GLenum Texture::SetParameteri(
    const FeatureInfo* feature_info, GLenum pname, GLint param) {
  DCHECK(feature_info);

  if (target_ == GL_TEXTURE_EXTERNAL_OES ||
      target_ == GL_TEXTURE_RECTANGLE_ARB) {
    if (pname == GL_TEXTURE_MIN_FILTER &&
        (param != GL_NEAREST && param != GL_LINEAR))
      return GL_INVALID_ENUM;
    if ((pname == GL_TEXTURE_WRAP_S || pname == GL_TEXTURE_WRAP_T) &&
        param != GL_CLAMP_TO_EDGE)
      return GL_INVALID_ENUM;
  }

  switch (pname) {
    case GL_TEXTURE_MIN_FILTER:
      if (!feature_info->validators()->texture_min_filter_mode.IsValid(param)) {
        return GL_INVALID_ENUM;
      }
      min_filter_ = param;
      break;
    case GL_TEXTURE_MAG_FILTER:
      if (!feature_info->validators()->texture_mag_filter_mode.IsValid(param)) {
        return GL_INVALID_ENUM;
      }
      mag_filter_ = param;
      break;
    case GL_TEXTURE_POOL_CHROMIUM:
      if (!feature_info->validators()->texture_pool.IsValid(param)) {
        return GL_INVALID_ENUM;
      }
      GetMemTracker()->TrackMemFree(estimated_size());
      pool_ = param;
      GetMemTracker()->TrackMemAlloc(estimated_size());
      break;
    case GL_TEXTURE_WRAP_S:
      if (!feature_info->validators()->texture_wrap_mode.IsValid(param)) {
        return GL_INVALID_ENUM;
      }
      wrap_s_ = param;
      break;
    case GL_TEXTURE_WRAP_T:
      if (!feature_info->validators()->texture_wrap_mode.IsValid(param)) {
        return GL_INVALID_ENUM;
      }
      wrap_t_ = param;
      break;
    case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      if (param < 1) {
        return GL_INVALID_VALUE;
      }
      break;
    case GL_TEXTURE_USAGE_ANGLE:
      if (!feature_info->validators()->texture_usage.IsValid(param)) {
        return GL_INVALID_ENUM;
      }
      usage_ = param;
      break;
    default:
      NOTREACHED();
      return GL_INVALID_ENUM;
  }
  Update(feature_info);
  UpdateCleared();
  UpdateCanRenderCondition();
  return GL_NO_ERROR;
}

GLenum Texture::SetParameterf(
    const FeatureInfo* feature_info, GLenum pname, GLfloat param) {
  switch (pname) {
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_POOL_CHROMIUM:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_TEXTURE_USAGE_ANGLE:
      {
        GLint iparam = static_cast<GLint>(param);
        return SetParameteri(feature_info, pname, iparam);
      }
    case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      if (param < 1.f) {
        return GL_INVALID_VALUE;
      }
      break;
    default:
      NOTREACHED();
      return GL_INVALID_ENUM;
  }
  return GL_NO_ERROR;
}

void Texture::Update(const FeatureInfo* feature_info) {
  // Update npot status.
  // Assume GL_TEXTURE_EXTERNAL_OES textures are npot, all others
  npot_ = target_ == GL_TEXTURE_EXTERNAL_OES;

  if (level_infos_.empty()) {
    texture_complete_ = false;
    cube_complete_ = false;
    return;
  }

  // checks that the first mip of any face is npot.
  for (size_t ii = 0; ii < level_infos_.size(); ++ii) {
    const Texture::LevelInfo& info = level_infos_[ii][0];
    if (GLES2Util::IsNPOT(info.width) ||
        GLES2Util::IsNPOT(info.height) ||
        GLES2Util::IsNPOT(info.depth)) {
      npot_ = true;
      break;
    }
  }

  // Update texture_complete and cube_complete status.
  const Texture::LevelInfo& first_face = level_infos_[0][0];
  int levels_needed = TextureManager::ComputeMipMapCount(
      target_, first_face.width, first_face.height, first_face.depth);
  texture_complete_ =
      max_level_set_ >= (levels_needed - 1) && max_level_set_ >= 0;
  cube_complete_ = (level_infos_.size() == 6) &&
                   (first_face.width == first_face.height);

  if (first_face.width == 0 || first_face.height == 0) {
    texture_complete_ = false;
  }
  if (first_face.type == GL_FLOAT &&
      !feature_info->feature_flags().enable_texture_float_linear &&
      (min_filter_ != GL_NEAREST_MIPMAP_NEAREST ||
       mag_filter_ != GL_NEAREST)) {
    texture_complete_ = false;
  } else if (first_face.type == GL_HALF_FLOAT_OES &&
             !feature_info->feature_flags().enable_texture_half_float_linear &&
             (min_filter_ != GL_NEAREST_MIPMAP_NEAREST ||
              mag_filter_ != GL_NEAREST)) {
    texture_complete_ = false;
  }
  for (size_t ii = 0;
       ii < level_infos_.size() && (cube_complete_ || texture_complete_);
       ++ii) {
    const Texture::LevelInfo& level0 = level_infos_[ii][0];
    if (level0.target == 0 ||
        level0.width != first_face.width ||
        level0.height != first_face.height ||
        level0.depth != 1 ||
        level0.internal_format != first_face.internal_format ||
        level0.format != first_face.format ||
        level0.type != first_face.type) {
      cube_complete_ = false;
    }
    // Get level0 dimensions
    GLsizei width = level0.width;
    GLsizei height = level0.height;
    GLsizei depth = level0.depth;
    for (GLint jj = 1; jj < levels_needed; ++jj) {
      // compute required size for mip.
      width = std::max(1, width >> 1);
      height = std::max(1, height >> 1);
      depth = std::max(1, depth >> 1);
      const Texture::LevelInfo& info = level_infos_[ii][jj];
      if (info.target == 0 ||
          info.width != width ||
          info.height != height ||
          info.depth != depth ||
          info.internal_format != level0.internal_format ||
          info.format != level0.format ||
          info.type != level0.type) {
        texture_complete_ = false;
        break;
      }
    }
  }
}

bool Texture::ClearRenderableLevels(GLES2Decoder* decoder) {
  DCHECK(decoder);
  if (cleared_) {
    return true;
  }

  const Texture::LevelInfo& first_face = level_infos_[0][0];
  int levels_needed = TextureManager::ComputeMipMapCount(
      target_, first_face.width, first_face.height, first_face.depth);

  for (size_t ii = 0; ii < level_infos_.size(); ++ii) {
    for (GLint jj = 0; jj < levels_needed; ++jj) {
      Texture::LevelInfo& info = level_infos_[ii][jj];
      if (info.target != 0) {
        if (!ClearLevel(decoder, info.target, jj)) {
          return false;
        }
      }
    }
  }
  UpdateSafeToRenderFrom(true);
  return true;
}

bool Texture::IsLevelCleared(GLenum target, GLint level) const {
  size_t face_index = GLTargetToFaceIndex(target);
  if (face_index >= level_infos_.size() ||
      level >= static_cast<GLint>(level_infos_[face_index].size())) {
    return true;
  }

  const Texture::LevelInfo& info = level_infos_[face_index][level];

  return info.cleared;
}

void Texture::InitTextureMaxAnisotropyIfNeeded(GLenum target) {
  if (texture_max_anisotropy_initialized_)
    return;
  texture_max_anisotropy_initialized_ = true;
  GLfloat params[] = { 1.0f };
  glTexParameterfv(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, params);
}

bool Texture::ClearLevel(
    GLES2Decoder* decoder, GLenum target, GLint level) {
  DCHECK(decoder);
  size_t face_index = GLTargetToFaceIndex(target);
  if (face_index >= level_infos_.size() ||
      level >= static_cast<GLint>(level_infos_[face_index].size())) {
    return true;
  }

  Texture::LevelInfo& info = level_infos_[face_index][level];

  DCHECK(target == info.target);

  if (info.target == 0 ||
      info.cleared ||
      info.width == 0 ||
      info.height == 0 ||
      info.depth == 0) {
    return true;
  }

  // NOTE: It seems kind of gross to call back into the decoder for this
  // but only the decoder knows all the state (like unpack_alignment_) that's
  // needed to be able to call GL correctly.
  bool cleared = decoder->ClearLevel(
      service_id_, target_, info.target, info.level, info.internal_format,
      info.format, info.type, info.width, info.height, immutable_);
  UpdateMipCleared(&info, cleared);
  return info.cleared;
}

void Texture::SetLevelImage(
    const FeatureInfo* feature_info,
    GLenum target,
    GLint level,
    gfx::GLImage* image) {
  DCHECK_GE(level, 0);
  DCHECK_LT(static_cast<size_t>(GLTargetToFaceIndex(target)),
            level_infos_.size());
  DCHECK_LT(static_cast<size_t>(level),
            level_infos_[GLTargetToFaceIndex(target)].size());
  Texture::LevelInfo& info =
      level_infos_[GLTargetToFaceIndex(target)][level];
  DCHECK_EQ(info.target, target);
  DCHECK_EQ(info.level, level);
  info.image = image;
  UpdateCanRenderCondition();
  UpdateHasImages();
}

gfx::GLImage* Texture::GetLevelImage(GLint target, GLint level) const {
  if (target != GL_TEXTURE_2D && target != GL_TEXTURE_EXTERNAL_OES &&
      target != GL_TEXTURE_RECTANGLE_ARB) {
    return NULL;
  }

  size_t face_index = GLTargetToFaceIndex(target);
  if (level >= 0 && face_index < level_infos_.size() &&
      static_cast<size_t>(level) < level_infos_[face_index].size()) {
    const LevelInfo& info = level_infos_[GLTargetToFaceIndex(target)][level];
    if (info.target != 0) {
      return info.image.get();
    }
  }
  return 0;
}

void Texture::OnWillModifyPixels() {
  gfx::GLImage* image = GetLevelImage(target(), 0);
  if (image)
    image->WillModifyTexImage();
}

void Texture::OnDidModifyPixels() {
  gfx::GLImage* image = GetLevelImage(target(), 0);
  if (image)
    image->DidModifyTexImage();
}

TextureRef::TextureRef(TextureManager* manager,
                       GLuint client_id,
                       Texture* texture)
    : manager_(manager),
      texture_(texture),
      client_id_(client_id),
      num_observers_(0) {
  DCHECK(manager_);
  DCHECK(texture_);
  texture_->AddTextureRef(this);
  manager_->StartTracking(this);
}

scoped_refptr<TextureRef> TextureRef::Create(TextureManager* manager,
                                             GLuint client_id,
                                             GLuint service_id) {
  return new TextureRef(manager, client_id, new Texture(service_id));
}

TextureRef::~TextureRef() {
  manager_->StopTracking(this);
  texture_->RemoveTextureRef(this, manager_->have_context_);
  manager_ = NULL;
}

TextureManager::TextureManager(MemoryTracker* memory_tracker,
                               FeatureInfo* feature_info,
                               GLint max_texture_size,
                               GLint max_cube_map_texture_size,
                               bool use_default_textures)
    : memory_tracker_managed_(
          new MemoryTypeTracker(memory_tracker, MemoryTracker::kManaged)),
      memory_tracker_unmanaged_(
          new MemoryTypeTracker(memory_tracker, MemoryTracker::kUnmanaged)),
      feature_info_(feature_info),
      framebuffer_manager_(NULL),
      max_texture_size_(max_texture_size),
      max_cube_map_texture_size_(max_cube_map_texture_size),
      max_levels_(ComputeMipMapCount(GL_TEXTURE_2D,
                                     max_texture_size,
                                     max_texture_size,
                                     max_texture_size)),
      max_cube_map_levels_(ComputeMipMapCount(GL_TEXTURE_CUBE_MAP,
                                              max_cube_map_texture_size,
                                              max_cube_map_texture_size,
                                              max_cube_map_texture_size)),
      use_default_textures_(use_default_textures),
      num_unrenderable_textures_(0),
      num_unsafe_textures_(0),
      num_uncleared_mips_(0),
      num_images_(0),
      texture_count_(0),
      have_context_(true) {
  for (int ii = 0; ii < kNumDefaultTextures; ++ii) {
    black_texture_ids_[ii] = 0;
  }
}

bool TextureManager::Initialize() {
  // TODO(gman): The default textures have to be real textures, not the 0
  // texture because we simulate non shared resources on top of shared
  // resources and all contexts that share resource share the same default
  // texture.
  default_textures_[kTexture2D] = CreateDefaultAndBlackTextures(
      GL_TEXTURE_2D, &black_texture_ids_[kTexture2D]);
  default_textures_[kCubeMap] = CreateDefaultAndBlackTextures(
      GL_TEXTURE_CUBE_MAP, &black_texture_ids_[kCubeMap]);

  if (feature_info_->feature_flags().oes_egl_image_external) {
    default_textures_[kExternalOES] = CreateDefaultAndBlackTextures(
        GL_TEXTURE_EXTERNAL_OES, &black_texture_ids_[kExternalOES]);
  }

  if (feature_info_->feature_flags().arb_texture_rectangle) {
    default_textures_[kRectangleARB] = CreateDefaultAndBlackTextures(
        GL_TEXTURE_RECTANGLE_ARB, &black_texture_ids_[kRectangleARB]);
  }

  return true;
}

scoped_refptr<TextureRef>
    TextureManager::CreateDefaultAndBlackTextures(
        GLenum target,
        GLuint* black_texture) {
  static uint8 black[] = {0, 0, 0, 255};

  // Sampling a texture not associated with any EGLImage sibling will return
  // black values according to the spec.
  bool needs_initialization = (target != GL_TEXTURE_EXTERNAL_OES);
  bool needs_faces = (target == GL_TEXTURE_CUBE_MAP);

  // Make default textures and texture for replacing non-renderable textures.
  GLuint ids[2];
  const unsigned long num_ids = use_default_textures_ ? 2 : 1;
  glGenTextures(num_ids, ids);
  for (unsigned long ii = 0; ii < num_ids; ++ii) {
    glBindTexture(target, ids[ii]);
    if (needs_initialization) {
      if (needs_faces) {
        for (int jj = 0; jj < GLES2Util::kNumFaces; ++jj) {
          glTexImage2D(GLES2Util::IndexToGLFaceTarget(jj), 0, GL_RGBA, 1, 1, 0,
                       GL_RGBA, GL_UNSIGNED_BYTE, black);
        }
      } else {
        glTexImage2D(target, 0, GL_RGBA, 1, 1, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, black);
      }
    }
  }
  glBindTexture(target, 0);

  scoped_refptr<TextureRef> default_texture;
  if (use_default_textures_) {
    default_texture = TextureRef::Create(this, 0, ids[1]);
    SetTarget(default_texture.get(), target);
    if (needs_faces) {
      for (int ii = 0; ii < GLES2Util::kNumFaces; ++ii) {
        SetLevelInfo(default_texture.get(),
                     GLES2Util::IndexToGLFaceTarget(ii),
                     0,
                     GL_RGBA,
                     1,
                     1,
                     1,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     true);
      }
    } else {
      if (needs_initialization) {
        SetLevelInfo(default_texture.get(),
                     GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     1,
                     1,
                     1,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     true);
      } else {
        SetLevelInfo(default_texture.get(),
                     GL_TEXTURE_EXTERNAL_OES,
                     0,
                     GL_RGBA,
                     1,
                     1,
                     1,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     true);
      }
    }
  }

  *black_texture = ids[0];
  return default_texture;
}

bool TextureManager::ValidForTarget(
    GLenum target, GLint level, GLsizei width, GLsizei height, GLsizei depth) {
  GLsizei max_size = MaxSizeForTarget(target) >> level;
  return level >= 0 &&
         width >= 0 &&
         height >= 0 &&
         depth >= 0 &&
         level < MaxLevelsForTarget(target) &&
         width <= max_size &&
         height <= max_size &&
         depth <= max_size &&
         (level == 0 || feature_info_->feature_flags().npot_ok ||
          (!GLES2Util::IsNPOT(width) &&
           !GLES2Util::IsNPOT(height) &&
           !GLES2Util::IsNPOT(depth))) &&
         (target != GL_TEXTURE_CUBE_MAP || (width == height && depth == 1)) &&
         (target != GL_TEXTURE_2D || (depth == 1));
}

void TextureManager::SetTarget(TextureRef* ref, GLenum target) {
  DCHECK(ref);
  ref->texture()
      ->SetTarget(feature_info_.get(), target, MaxLevelsForTarget(target));
}

void TextureManager::SetLevelCleared(TextureRef* ref,
                                     GLenum target,
                                     GLint level,
                                     bool cleared) {
  DCHECK(ref);
  ref->texture()->SetLevelCleared(target, level, cleared);
}

bool TextureManager::ClearRenderableLevels(
    GLES2Decoder* decoder, TextureRef* ref) {
  DCHECK(ref);
  return ref->texture()->ClearRenderableLevels(decoder);
}

bool TextureManager::ClearTextureLevel(
    GLES2Decoder* decoder, TextureRef* ref,
    GLenum target, GLint level) {
  DCHECK(ref);
  Texture* texture = ref->texture();
  if (texture->num_uncleared_mips() == 0) {
    return true;
  }
  bool result = texture->ClearLevel(decoder, target, level);
  texture->UpdateCleared();
  return result;
}

void TextureManager::SetLevelInfo(
    TextureRef* ref,
    GLenum target,
    GLint level,
    GLenum internal_format,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLint border,
    GLenum format,
    GLenum type,
    bool cleared) {
  DCHECK(ref);
  Texture* texture = ref->texture();

  texture->GetMemTracker()->TrackMemFree(texture->estimated_size());
  texture->SetLevelInfo(feature_info_.get(),
                        target,
                        level,
                        internal_format,
                        width,
                        height,
                        depth,
                        border,
                        format,
                        type,
                        cleared);
  texture->GetMemTracker()->TrackMemAlloc(texture->estimated_size());
}

Texture* TextureManager::Produce(TextureRef* ref) {
  DCHECK(ref);
  return ref->texture();
}

TextureRef* TextureManager::Consume(
    GLuint client_id,
    Texture* texture) {
  DCHECK(client_id);
  scoped_refptr<TextureRef> ref(new TextureRef(this, client_id, texture));
  bool result = textures_.insert(std::make_pair(client_id, ref)).second;
  DCHECK(result);
  return ref.get();
}

void TextureManager::SetParameteri(
    const char* function_name, ErrorState* error_state,
    TextureRef* ref, GLenum pname, GLint param) {
  DCHECK(error_state);
  DCHECK(ref);
  Texture* texture = ref->texture();
  GLenum result = texture->SetParameteri(feature_info_.get(), pname, param);
  if (result != GL_NO_ERROR) {
    if (result == GL_INVALID_ENUM) {
      ERRORSTATE_SET_GL_ERROR_INVALID_ENUM(
          error_state, function_name, param, "param");
    } else {
      ERRORSTATE_SET_GL_ERROR_INVALID_PARAMI(
          error_state, result, function_name, pname, param);
    }
  } else {
    // Texture tracking pools exist only for the command decoder, so
    // do not pass them on to the native GL implementation.
    if (pname != GL_TEXTURE_POOL_CHROMIUM) {
      glTexParameteri(texture->target(), pname, param);
    }
  }
}

void TextureManager::SetParameterf(
    const char* function_name, ErrorState* error_state,
    TextureRef* ref, GLenum pname, GLfloat param) {
  DCHECK(error_state);
  DCHECK(ref);
  Texture* texture = ref->texture();
  GLenum result = texture->SetParameterf(feature_info_.get(), pname, param);
  if (result != GL_NO_ERROR) {
    if (result == GL_INVALID_ENUM) {
      ERRORSTATE_SET_GL_ERROR_INVALID_ENUM(
          error_state, function_name, param, "param");
    } else {
      ERRORSTATE_SET_GL_ERROR_INVALID_PARAMF(
          error_state, result, function_name, pname, param);
    }
  } else {
    // Texture tracking pools exist only for the command decoder, so
    // do not pass them on to the native GL implementation.
    if (pname != GL_TEXTURE_POOL_CHROMIUM) {
      glTexParameterf(texture->target(), pname, param);
    }
  }
}

bool TextureManager::MarkMipmapsGenerated(TextureRef* ref) {
  DCHECK(ref);
  Texture* texture = ref->texture();
  texture->GetMemTracker()->TrackMemFree(texture->estimated_size());
  bool result = texture->MarkMipmapsGenerated(feature_info_.get());
  texture->GetMemTracker()->TrackMemAlloc(texture->estimated_size());
  return result;
}

TextureRef* TextureManager::CreateTexture(
    GLuint client_id, GLuint service_id) {
  DCHECK_NE(0u, service_id);
  scoped_refptr<TextureRef> ref(TextureRef::Create(
      this, client_id, service_id));
  std::pair<TextureMap::iterator, bool> result =
      textures_.insert(std::make_pair(client_id, ref));
  DCHECK(result.second);
  return ref.get();
}

TextureRef* TextureManager::GetTexture(
    GLuint client_id) const {
  TextureMap::const_iterator it = textures_.find(client_id);
  return it != textures_.end() ? it->second.get() : NULL;
}

void TextureManager::RemoveTexture(GLuint client_id) {
  TextureMap::iterator it = textures_.find(client_id);
  if (it != textures_.end()) {
    it->second->reset_client_id();
    textures_.erase(it);
  }
}

void TextureManager::StartTracking(TextureRef* ref) {
  Texture* texture = ref->texture();
  ++texture_count_;
  num_uncleared_mips_ += texture->num_uncleared_mips();
  if (!texture->SafeToRenderFrom())
    ++num_unsafe_textures_;
  if (!texture->CanRender(feature_info_.get()))
    ++num_unrenderable_textures_;
  if (texture->HasImages())
    ++num_images_;
}

void TextureManager::StopTracking(TextureRef* ref) {
  if (ref->num_observers()) {
    for (unsigned int i = 0; i < destruction_observers_.size(); i++) {
      destruction_observers_[i]->OnTextureRefDestroying(ref);
    }
    DCHECK_EQ(ref->num_observers(), 0);
  }

  Texture* texture = ref->texture();

  --texture_count_;
  if (texture->HasImages()) {
    DCHECK_NE(0, num_images_);
    --num_images_;
  }
  if (!texture->CanRender(feature_info_.get())) {
    DCHECK_NE(0, num_unrenderable_textures_);
    --num_unrenderable_textures_;
  }
  if (!texture->SafeToRenderFrom()) {
    DCHECK_NE(0, num_unsafe_textures_);
    --num_unsafe_textures_;
  }
  num_uncleared_mips_ -= texture->num_uncleared_mips();
  DCHECK_GE(num_uncleared_mips_, 0);
}

MemoryTypeTracker* TextureManager::GetMemTracker(GLenum tracking_pool) {
  switch(tracking_pool) {
    case GL_TEXTURE_POOL_MANAGED_CHROMIUM:
      return memory_tracker_managed_.get();
      break;
    case GL_TEXTURE_POOL_UNMANAGED_CHROMIUM:
      return memory_tracker_unmanaged_.get();
      break;
    default:
      break;
  }
  NOTREACHED();
  return NULL;
}

Texture* TextureManager::GetTextureForServiceId(GLuint service_id) const {
  // This doesn't need to be fast. It's only used during slow queries.
  for (TextureMap::const_iterator it = textures_.begin();
       it != textures_.end(); ++it) {
    Texture* texture = it->second->texture();
    if (texture->service_id() == service_id)
      return texture;
  }
  return NULL;
}

GLsizei TextureManager::ComputeMipMapCount(GLenum target,
                                           GLsizei width,
                                           GLsizei height,
                                           GLsizei depth) {
  switch (target) {
    case GL_TEXTURE_EXTERNAL_OES:
      return 1;
    default:
      return 1 +
             base::bits::Log2Floor(std::max(std::max(width, height), depth));
  }
}

void TextureManager::SetLevelImage(
    TextureRef* ref,
    GLenum target,
    GLint level,
    gfx::GLImage* image) {
  DCHECK(ref);
  ref->texture()->SetLevelImage(feature_info_.get(), target, level, image);
}

void TextureManager::AddToSignature(
    TextureRef* ref,
    GLenum target,
    GLint level,
    std::string* signature) const {
  ref->texture()->AddToSignature(feature_info_.get(), target, level, signature);
}

void TextureManager::UpdateSafeToRenderFrom(int delta) {
  num_unsafe_textures_ += delta;
  DCHECK_GE(num_unsafe_textures_, 0);
}

void TextureManager::UpdateUnclearedMips(int delta) {
  num_uncleared_mips_ += delta;
  DCHECK_GE(num_uncleared_mips_, 0);
}

void TextureManager::UpdateCanRenderCondition(
    Texture::CanRenderCondition old_condition,
    Texture::CanRenderCondition new_condition) {
  if (old_condition == Texture::CAN_RENDER_NEVER ||
      (old_condition == Texture::CAN_RENDER_ONLY_IF_NPOT &&
       !feature_info_->feature_flags().npot_ok)) {
    DCHECK_GT(num_unrenderable_textures_, 0);
    --num_unrenderable_textures_;
  }
  if (new_condition == Texture::CAN_RENDER_NEVER ||
      (new_condition == Texture::CAN_RENDER_ONLY_IF_NPOT &&
       !feature_info_->feature_flags().npot_ok))
    ++num_unrenderable_textures_;
}

void TextureManager::UpdateNumImages(int delta) {
  num_images_ += delta;
  DCHECK_GE(num_images_, 0);
}

void TextureManager::IncFramebufferStateChangeCount() {
  if (framebuffer_manager_)
    framebuffer_manager_->IncFramebufferStateChangeCount();
}

bool TextureManager::ValidateFormatAndTypeCombination(
    ErrorState* error_state, const char* function_name, GLenum format,
    GLenum type) {
  if (!feature_info_->GetTextureFormatValidator(format).IsValid(type)) {
    ERRORSTATE_SET_GL_ERROR(
        error_state, GL_INVALID_OPERATION, function_name,
        (std::string("invalid type ") +
         GLES2Util::GetStringEnum(type) + " for format " +
         GLES2Util::GetStringEnum(format)).c_str());
    return false;
  }
  return true;
}

bool TextureManager::ValidateTextureParameters(
    ErrorState* error_state, const char* function_name,
    GLenum format, GLenum type, GLenum internal_format, GLint level) {
  const Validators* validators = feature_info_->validators();
  if (!validators->texture_format.IsValid(format)) {
    ERRORSTATE_SET_GL_ERROR_INVALID_ENUM(
        error_state, function_name, format, "format");
    return false;
  }
  if (!validators->pixel_type.IsValid(type)) {
    ERRORSTATE_SET_GL_ERROR_INVALID_ENUM(
        error_state, function_name, type, "type");
    return false;
  }
  if (format != internal_format &&
      !((internal_format == GL_RGBA32F && format == GL_RGBA) ||
        (internal_format == GL_RGB32F && format == GL_RGB))) {
    ERRORSTATE_SET_GL_ERROR(
        error_state, GL_INVALID_OPERATION, function_name,
        "format != internalformat");
    return false;
  }
  uint32 channels = GLES2Util::GetChannelsForFormat(format);
  if ((channels & (GLES2Util::kDepth | GLES2Util::kStencil)) != 0 && level) {
    ERRORSTATE_SET_GL_ERROR(
        error_state, GL_INVALID_OPERATION, function_name,
        (std::string("invalid format ") + GLES2Util::GetStringEnum(format) +
         " for level != 0").c_str());
    return false;
  }
  return ValidateFormatAndTypeCombination(error_state, function_name,
      format, type);
}

// Gets the texture id for a given target.
TextureRef* TextureManager::GetTextureInfoForTarget(
    ContextState* state, GLenum target) {
  TextureUnit& unit = state->texture_units[state->active_texture_unit];
  TextureRef* texture = NULL;
  switch (target) {
    case GL_TEXTURE_2D:
      texture = unit.bound_texture_2d.get();
      break;
    case GL_TEXTURE_CUBE_MAP:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      texture = unit.bound_texture_cube_map.get();
      break;
    case GL_TEXTURE_EXTERNAL_OES:
      texture = unit.bound_texture_external_oes.get();
      break;
    case GL_TEXTURE_RECTANGLE_ARB:
      texture = unit.bound_texture_rectangle_arb.get();
      break;
    default:
      NOTREACHED();
      return NULL;
  }
  return texture;
}

TextureRef* TextureManager::GetTextureInfoForTargetUnlessDefault(
    ContextState* state, GLenum target) {
  TextureRef* texture = GetTextureInfoForTarget(state, target);
  if (!texture)
    return NULL;
  if (texture == GetDefaultTextureInfo(target))
    return NULL;
  return texture;
}

bool TextureManager::ValidateTexImage2D(
    ContextState* state,
    const char* function_name,
    const DoTextImage2DArguments& args,
    TextureRef** texture_ref) {
  ErrorState* error_state = state->GetErrorState();
  const Validators* validators = feature_info_->validators();
  if (!validators->texture_target.IsValid(args.target)) {
    ERRORSTATE_SET_GL_ERROR_INVALID_ENUM(
        error_state, function_name, args.target, "target");
    return false;
  }
  if (!validators->texture_internal_format.IsValid(args.internal_format)) {
    ERRORSTATE_SET_GL_ERROR_INVALID_ENUM(
        error_state, function_name, args.internal_format,
        "internalformat");
    return false;
  }
  if (!ValidateTextureParameters(
      error_state, function_name, args.format, args.type,
      args.internal_format, args.level)) {
    return false;
  }
  if (!ValidForTarget(args.target, args.level, args.width, args.height, 1) ||
      args.border != 0) {
    ERRORSTATE_SET_GL_ERROR(
        error_state, GL_INVALID_VALUE, function_name,
        "dimensions out of range");
    return false;
  }
  if ((GLES2Util::GetChannelsForFormat(args.format) &
       (GLES2Util::kDepth | GLES2Util::kStencil)) != 0 && args.pixels) {
    ERRORSTATE_SET_GL_ERROR(
        error_state, GL_INVALID_OPERATION,
        function_name, "can not supply data for depth or stencil textures");
    return false;
  }

  TextureRef* local_texture_ref = GetTextureInfoForTarget(state, args.target);
  if (!local_texture_ref) {
    ERRORSTATE_SET_GL_ERROR(
        error_state, GL_INVALID_OPERATION, function_name,
        "unknown texture for target");
    return false;
  }
  if (local_texture_ref->texture()->IsImmutable()) {
    ERRORSTATE_SET_GL_ERROR(
        error_state, GL_INVALID_OPERATION, function_name,
        "texture is immutable");
    return false;
  }

  // TODO - verify that using the managed vs unmanaged does not matter.
  // They both use the same MemoryTracker, and this call just re-routes
  // to it.
  if (!memory_tracker_managed_->EnsureGPUMemoryAvailable(args.pixels_size)) {
    ERRORSTATE_SET_GL_ERROR(error_state, GL_OUT_OF_MEMORY, function_name,
                            "out of memory");
    return false;
  }

  // Write the TextureReference since this is valid.
  *texture_ref = local_texture_ref;
  return true;
}

void TextureManager::ValidateAndDoTexImage2D(
    DecoderTextureState* texture_state,
    ContextState* state,
    DecoderFramebufferState* framebuffer_state,
    const DoTextImage2DArguments& args) {
  TextureRef* texture_ref;
  if (!ValidateTexImage2D(state, "glTexImage2D", args, &texture_ref)) {
    return;
  }

  DoTexImage2D(texture_state, state->GetErrorState(), framebuffer_state,
               texture_ref, args);
}

void TextureManager::DoTexImage2D(
    DecoderTextureState* texture_state,
    ErrorState* error_state,
    DecoderFramebufferState* framebuffer_state,
    TextureRef* texture_ref,
    const DoTextImage2DArguments& args) {
  Texture* texture = texture_ref->texture();
  GLsizei tex_width = 0;
  GLsizei tex_height = 0;
  GLenum tex_type = 0;
  GLenum tex_format = 0;
  bool level_is_same =
      texture->GetLevelSize(args.target, args.level, &tex_width, &tex_height) &&
      texture->GetLevelType(args.target, args.level, &tex_type, &tex_format) &&
      args.width == tex_width && args.height == tex_height &&
      args.type == tex_type && args.format == tex_format;

  if (level_is_same && !args.pixels) {
    // Just set the level texture but mark the texture as uncleared.
    SetLevelInfo(
        texture_ref,
        args.target, args.level, args.internal_format, args.width, args.height,
        1, args.border, args.format, args.type, false);
    texture_state->tex_image_2d_failed = false;
    return;
  }

  if (texture->IsAttachedToFramebuffer()) {
    framebuffer_state->clear_state_dirty = true;
  }

  if (texture_state->texsubimage2d_faster_than_teximage2d &&
      level_is_same && args.pixels) {
    {
      ScopedTextureUploadTimer timer(texture_state);
      glTexSubImage2D(args.target, args.level, 0, 0, args.width, args.height,
                      args.format, args.type, args.pixels);
    }
    SetLevelCleared(texture_ref, args.target, args.level, true);
    texture_state->tex_image_2d_failed = false;
    return;
  }

  ERRORSTATE_COPY_REAL_GL_ERRORS_TO_WRAPPER(error_state, "glTexImage2D");
  {
    ScopedTextureUploadTimer timer(texture_state);
    glTexImage2D(
        args.target, args.level, args.internal_format, args.width, args.height,
        args.border, args.format, args.type, args.pixels);
  }
  GLenum error = ERRORSTATE_PEEK_GL_ERROR(error_state, "glTexImage2D");
  if (error == GL_NO_ERROR) {
    SetLevelInfo(
        texture_ref,
        args.target, args.level, args.internal_format, args.width, args.height,
        1, args.border, args.format, args.type, args.pixels != NULL);
    texture_state->tex_image_2d_failed = false;
  }
}

ScopedTextureUploadTimer::ScopedTextureUploadTimer(
    DecoderTextureState* texture_state)
    : texture_state_(texture_state),
      begin_time_(base::TimeTicks::HighResNow()) {
}

ScopedTextureUploadTimer::~ScopedTextureUploadTimer() {
  texture_state_->texture_upload_count++;
  texture_state_->total_texture_upload_time +=
      base::TimeTicks::HighResNow() - begin_time_;
}

}  // namespace gles2
}  // namespace gpu
