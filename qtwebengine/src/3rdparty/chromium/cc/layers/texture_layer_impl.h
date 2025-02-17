// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_TEXTURE_LAYER_IMPL_H_
#define CC_LAYERS_TEXTURE_LAYER_IMPL_H_

#include <string>

#include "base/callback.h"
#include "cc/base/cc_export.h"
#include "cc/layers/layer_impl.h"

namespace cc {
class SingleReleaseCallback;
class ScopedResource;

class CC_EXPORT TextureLayerImpl : public LayerImpl {
 public:
  static scoped_ptr<TextureLayerImpl> Create(LayerTreeImpl* tree_impl, int id) {
    return make_scoped_ptr(new TextureLayerImpl(tree_impl, id));
  }
  virtual ~TextureLayerImpl();

  virtual scoped_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* layer_tree_impl)
      OVERRIDE;
  virtual void PushPropertiesTo(LayerImpl* layer) OVERRIDE;

  virtual bool WillDraw(DrawMode draw_mode,
                        ResourceProvider* resource_provider) OVERRIDE;
  virtual void AppendQuads(QuadSink* quad_sink,
                           AppendQuadsData* append_quads_data) OVERRIDE;
  virtual Region VisibleContentOpaqueRegion() const OVERRIDE;
  virtual void ReleaseResources() OVERRIDE;

  // These setter methods don't cause any implicit damage, so the texture client
  // must explicitly invalidate if they intend to cause a visible change in the
  // layer's output.
  void SetTextureId(unsigned id);
  void SetPremultipliedAlpha(bool premultiplied_alpha);
  void SetBlendBackgroundColor(bool blend);
  void SetFlipped(bool flipped);
  void SetUVTopLeft(const gfx::PointF top_left);
  void SetUVBottomRight(const gfx::PointF bottom_right);

  // 1--2
  // |  |
  // 0--3
  void SetVertexOpacity(const float vertex_opacity[4]);

  void SetTextureMailbox(const TextureMailbox& mailbox,
                         scoped_ptr<SingleReleaseCallback> release_callback);

 private:
  TextureLayerImpl(LayerTreeImpl* tree_impl, int id);

  virtual const char* LayerTypeAsString() const OVERRIDE;
  void FreeTextureMailbox();

  ResourceProvider::ResourceId external_texture_resource_;
  bool premultiplied_alpha_;
  bool blend_background_color_;
  bool flipped_;
  gfx::PointF uv_top_left_;
  gfx::PointF uv_bottom_right_;
  float vertex_opacity_[4];
  // This is a resource that's a GL copy of a software texture mailbox.
  scoped_ptr<ScopedResource> texture_copy_;

  TextureMailbox texture_mailbox_;
  scoped_ptr<SingleReleaseCallback> release_callback_;
  bool own_mailbox_;
  bool valid_texture_copy_;

  DISALLOW_COPY_AND_ASSIGN(TextureLayerImpl);
};

}  // namespace cc

#endif  // CC_LAYERS_TEXTURE_LAYER_IMPL_H_
