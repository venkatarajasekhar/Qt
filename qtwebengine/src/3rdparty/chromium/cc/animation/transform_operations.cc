// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/transform_operations.h"

#include <algorithm>

#include "ui/gfx/animation/tween.h"
#include "ui/gfx/box_f.h"
#include "ui/gfx/transform_util.h"
#include "ui/gfx/vector3d_f.h"

namespace cc {

TransformOperations::TransformOperations()
    : decomposed_transform_dirty_(true) {
}

TransformOperations::TransformOperations(const TransformOperations& other) {
  operations_ = other.operations_;
  decomposed_transform_dirty_ = other.decomposed_transform_dirty_;
  if (!decomposed_transform_dirty_) {
    decomposed_transform_.reset(
        new gfx::DecomposedTransform(*other.decomposed_transform_.get()));
  }
}

TransformOperations::~TransformOperations() {
}

gfx::Transform TransformOperations::Apply() const {
  gfx::Transform to_return;
  for (size_t i = 0; i < operations_.size(); ++i)
    to_return.PreconcatTransform(operations_[i].matrix);
  return to_return;
}

gfx::Transform TransformOperations::Blend(const TransformOperations& from,
                                          SkMScalar progress) const {
  gfx::Transform to_return;
  BlendInternal(from, progress, &to_return);
  return to_return;
}

bool TransformOperations::BlendedBoundsForBox(const gfx::BoxF& box,
                                              const TransformOperations& from,
                                              SkMScalar min_progress,
                                              SkMScalar max_progress,
                                              gfx::BoxF* bounds) const {
  *bounds = box;

  bool from_identity = from.IsIdentity();
  bool to_identity = IsIdentity();
  if (from_identity && to_identity)
    return true;

  if (!MatchesTypes(from))
    return false;

  size_t num_operations =
      std::max(from_identity ? 0 : from.operations_.size(),
               to_identity ? 0 : operations_.size());

  // Because we are squashing all of the matrices together when applying
  // them to the animation, we must apply them in reverse order when
  // not squashing them.
  for (int i = num_operations - 1; i >= 0; --i) {
    gfx::BoxF bounds_for_operation;
    const TransformOperation* from_op =
        from_identity ? NULL : &from.operations_[i];
    const TransformOperation* to_op = to_identity ? NULL : &operations_[i];
    if (!TransformOperation::BlendedBoundsForBox(*bounds,
                                                 from_op,
                                                 to_op,
                                                 min_progress,
                                                 max_progress,
                                                 &bounds_for_operation))
      return false;
    *bounds = bounds_for_operation;
  }

  return true;
}

bool TransformOperations::AffectsScale() const {
  for (size_t i = 0; i < operations_.size(); ++i) {
    if (operations_[i].type == TransformOperation::TransformOperationScale)
      return true;
    if (operations_[i].type == TransformOperation::TransformOperationMatrix &&
        !operations_[i].matrix.IsIdentityOrTranslation())
      return true;
  }
  return false;
}

bool TransformOperations::IsTranslation() const {
  for (size_t i = 0; i < operations_.size(); ++i) {
    switch (operations_[i].type) {
      case TransformOperation::TransformOperationIdentity:
      case TransformOperation::TransformOperationTranslate:
        continue;
      case TransformOperation::TransformOperationMatrix:
        if (!operations_[i].matrix.IsIdentityOrTranslation())
          return false;
        continue;
      case TransformOperation::TransformOperationRotate:
      case TransformOperation::TransformOperationScale:
      case TransformOperation::TransformOperationSkew:
      case TransformOperation::TransformOperationPerspective:
        return false;
    }
  }
  return true;
}

bool TransformOperations::MaximumScale(const TransformOperations& from,
                                       SkMScalar min_progress,
                                       SkMScalar max_progress,
                                       float* max_scale) const {
  if (!MatchesTypes(from))
    return false;

  gfx::Vector3dF from_scale;
  gfx::Vector3dF to_scale;

  if (!from.ScaleComponent(&from_scale) || !ScaleComponent(&to_scale))
    return false;

  gfx::Vector3dF scale_at_min_progress(
      std::abs(gfx::Tween::FloatValueBetween(
          min_progress, from_scale.x(), to_scale.x())),
      std::abs(gfx::Tween::FloatValueBetween(
          min_progress, from_scale.y(), to_scale.y())),
      std::abs(gfx::Tween::FloatValueBetween(
          min_progress, from_scale.z(), to_scale.z())));
  gfx::Vector3dF scale_at_max_progress(
      std::abs(gfx::Tween::FloatValueBetween(
          max_progress, from_scale.x(), to_scale.x())),
      std::abs(gfx::Tween::FloatValueBetween(
          max_progress, from_scale.y(), to_scale.y())),
      std::abs(gfx::Tween::FloatValueBetween(
          max_progress, from_scale.z(), to_scale.z())));

  gfx::Vector3dF max_scale_3d = scale_at_min_progress;
  max_scale_3d.SetToMax(scale_at_max_progress);
  *max_scale =
      std::max(max_scale_3d.x(), std::max(max_scale_3d.y(), max_scale_3d.z()));
  return true;
}

bool TransformOperations::ScaleComponent(gfx::Vector3dF* scale) const {
  *scale = gfx::Vector3dF(1.f, 1.f, 1.f);
  bool has_scale_component = false;
  for (size_t i = 0; i < operations_.size(); ++i) {
    switch (operations_[i].type) {
      case TransformOperation::TransformOperationIdentity:
      case TransformOperation::TransformOperationTranslate:
        continue;
      case TransformOperation::TransformOperationMatrix:
        if (!operations_[i].matrix.IsIdentityOrTranslation())
          return false;
        continue;
      case TransformOperation::TransformOperationRotate:
      case TransformOperation::TransformOperationSkew:
      case TransformOperation::TransformOperationPerspective:
        return false;
      case TransformOperation::TransformOperationScale:
        if (has_scale_component)
          return false;
        has_scale_component = true;
        scale->Scale(operations_[i].scale.x,
                     operations_[i].scale.y,
                     operations_[i].scale.z);
    }
  }
  return true;
}

bool TransformOperations::MatchesTypes(const TransformOperations& other) const {
  if (IsIdentity() || other.IsIdentity())
    return true;

  if (operations_.size() != other.operations_.size())
    return false;

  for (size_t i = 0; i < operations_.size(); ++i) {
    if (operations_[i].type != other.operations_[i].type
      && !operations_[i].IsIdentity()
      && !other.operations_[i].IsIdentity())
      return false;
  }

  return true;
}

bool TransformOperations::CanBlendWith(
    const TransformOperations& other) const {
  gfx::Transform dummy;
  return BlendInternal(other, 0.5, &dummy);
}

void TransformOperations::AppendTranslate(SkMScalar x,
                                          SkMScalar y,
                                          SkMScalar z) {
  TransformOperation to_add;
  to_add.matrix.Translate3d(x, y, z);
  to_add.type = TransformOperation::TransformOperationTranslate;
  to_add.translate.x = x;
  to_add.translate.y = y;
  to_add.translate.z = z;
  operations_.push_back(to_add);
  decomposed_transform_dirty_ = true;
}

void TransformOperations::AppendRotate(SkMScalar x,
                                       SkMScalar y,
                                       SkMScalar z,
                                       SkMScalar degrees) {
  TransformOperation to_add;
  to_add.matrix.RotateAbout(gfx::Vector3dF(x, y, z), degrees);
  to_add.type = TransformOperation::TransformOperationRotate;
  to_add.rotate.axis.x = x;
  to_add.rotate.axis.y = y;
  to_add.rotate.axis.z = z;
  to_add.rotate.angle = degrees;
  operations_.push_back(to_add);
  decomposed_transform_dirty_ = true;
}

void TransformOperations::AppendScale(SkMScalar x, SkMScalar y, SkMScalar z) {
  TransformOperation to_add;
  to_add.matrix.Scale3d(x, y, z);
  to_add.type = TransformOperation::TransformOperationScale;
  to_add.scale.x = x;
  to_add.scale.y = y;
  to_add.scale.z = z;
  operations_.push_back(to_add);
  decomposed_transform_dirty_ = true;
}

void TransformOperations::AppendSkew(SkMScalar x, SkMScalar y) {
  TransformOperation to_add;
  to_add.matrix.SkewX(x);
  to_add.matrix.SkewY(y);
  to_add.type = TransformOperation::TransformOperationSkew;
  to_add.skew.x = x;
  to_add.skew.y = y;
  operations_.push_back(to_add);
  decomposed_transform_dirty_ = true;
}

void TransformOperations::AppendPerspective(SkMScalar depth) {
  TransformOperation to_add;
  to_add.matrix.ApplyPerspectiveDepth(depth);
  to_add.type = TransformOperation::TransformOperationPerspective;
  to_add.perspective_depth = depth;
  operations_.push_back(to_add);
  decomposed_transform_dirty_ = true;
}

void TransformOperations::AppendMatrix(const gfx::Transform& matrix) {
  TransformOperation to_add;
  to_add.matrix = matrix;
  to_add.type = TransformOperation::TransformOperationMatrix;
  operations_.push_back(to_add);
  decomposed_transform_dirty_ = true;
}

void TransformOperations::AppendIdentity() {
  operations_.push_back(TransformOperation());
}

bool TransformOperations::IsIdentity() const {
  for (size_t i = 0; i < operations_.size(); ++i) {
    if (!operations_[i].IsIdentity())
      return false;
  }
  return true;
}

bool TransformOperations::BlendInternal(const TransformOperations& from,
                                        SkMScalar progress,
                                        gfx::Transform* result) const {
  bool from_identity = from.IsIdentity();
  bool to_identity = IsIdentity();
  if (from_identity && to_identity)
    return true;

  if (MatchesTypes(from)) {
    size_t num_operations =
        std::max(from_identity ? 0 : from.operations_.size(),
                 to_identity ? 0 : operations_.size());
    for (size_t i = 0; i < num_operations; ++i) {
      gfx::Transform blended;
      if (!TransformOperation::BlendTransformOperations(
          from_identity ? 0 : &from.operations_[i],
          to_identity ? 0 : &operations_[i],
          progress,
          &blended))
          return false;
      result->PreconcatTransform(blended);
    }
    return true;
  }

  if (!ComputeDecomposedTransform() || !from.ComputeDecomposedTransform())
    return false;

  gfx::DecomposedTransform to_return;
  if (!gfx::BlendDecomposedTransforms(&to_return,
                                      *decomposed_transform_.get(),
                                      *from.decomposed_transform_.get(),
                                      progress))
    return false;

  *result = ComposeTransform(to_return);
  return true;
}

bool TransformOperations::ComputeDecomposedTransform() const {
  if (decomposed_transform_dirty_) {
    if (!decomposed_transform_)
      decomposed_transform_.reset(new gfx::DecomposedTransform());
    gfx::Transform transform = Apply();
    if (!gfx::DecomposeTransform(decomposed_transform_.get(), transform))
      return false;
    decomposed_transform_dirty_ = false;
  }
  return true;
}

}  // namespace cc
