// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_INTERPOLATED_TRANSFORM_H_
#define UI_GFX_INTERPOLATED_TRANSFORM_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "ui/gfx/point.h"
#include "ui/gfx/point3_f.h"
#include "ui/gfx/transform.h"
#include "ui/gfx/transform_util.h"
#include "ui/gfx/vector3d_f.h"

namespace ui {

///////////////////////////////////////////////////////////////////////////////
// class InterpolatedTransform
//
// Abstract base class for transforms that animate over time. These
// interpolated transforms can be combined to allow for more sophisticated
// animations. For example, you might combine a rotation of 90 degrees between
// times 0 and 1, with a scale from 1 to 0.3 between times 0 and 0.25 and a
// scale from 0.3 to 1 from between times 0.75 and 1.
//
///////////////////////////////////////////////////////////////////////////////
class GFX_EXPORT InterpolatedTransform {
 public:
  InterpolatedTransform();
  // The interpolated transform varies only when t in (start_time, end_time).
  // If t <= start_time, Interpolate(t) will return the initial transform, and
  // if t >= end_time, Interpolate(t) will return the final transform.
  InterpolatedTransform(float start_time, float end_time);
  virtual ~InterpolatedTransform();

  // Returns the interpolated transform at time t. Note: not virtual.
  gfx::Transform Interpolate(float t) const;

  // The Intepolate ultimately returns the product of our transform at time t
  // and our child's transform at time t (if we have one).
  //
  // This function takes ownership of the passed InterpolatedTransform.
  void SetChild(InterpolatedTransform* child);

  // If the interpolated transform is reversed, Interpolate(t) will return
  // Interpolate(1 - t)
  void SetReversed(bool reversed) { reversed_ = reversed; }
  bool Reversed() const { return reversed_; }

 protected:
  // Calculates the interpolated transform without considering our child.
  virtual gfx::Transform InterpolateButDoNotCompose(float t) const = 0;

  // If time in (start_time_, end_time_], this function linearly interpolates
  // between start_value and end_value.  More precisely it returns
  // (1 - t) * start_value + t * end_value where
  // t = (start_time_ - time) / (end_time_ - start_time_).
  // If time < start_time_ it returns start_value, and if time >= end_time_
  // it returns end_value.
  float ValueBetween(float time, float start_value, float end_value) const;

  float start_time() const { return start_time_; }
  float end_time() const { return end_time_; }

 private:
  const float start_time_;
  const float end_time_;

  // The child transform. If you consider an interpolated transform as a
  // function of t. If, without a child, we are f(t), and our child is
  // g(t), then with a child we become f'(t) = f(t) * g(t). Using a child
  // transform, we can chain collections of transforms together.
  scoped_ptr<InterpolatedTransform> child_;

  bool reversed_;

  DISALLOW_COPY_AND_ASSIGN(InterpolatedTransform);
};

///////////////////////////////////////////////////////////////////////////////
// class InterpolatedRotation
//
// Represents an animated rotation.
//
///////////////////////////////////////////////////////////////////////////////
class GFX_EXPORT InterpolatedRotation : public InterpolatedTransform {
 public:
  InterpolatedRotation(float start_degrees, float end_degrees);
  InterpolatedRotation(float start_degrees,
                       float end_degrees,
                       float start_time,
                       float end_time);
  virtual ~InterpolatedRotation();

 protected:
  virtual gfx::Transform InterpolateButDoNotCompose(float t) const OVERRIDE;

 private:
  const float start_degrees_;
  const float end_degrees_;

  DISALLOW_COPY_AND_ASSIGN(InterpolatedRotation);
};

///////////////////////////////////////////////////////////////////////////////
// class InterpolatedAxisAngleRotation
//
// Represents an animated rotation.
//
///////////////////////////////////////////////////////////////////////////////
class GFX_EXPORT InterpolatedAxisAngleRotation : public InterpolatedTransform {
 public:
  InterpolatedAxisAngleRotation(const gfx::Vector3dF& axis,
                                float start_degrees,
                                float end_degrees);
  InterpolatedAxisAngleRotation(const gfx::Vector3dF& axis,
                                float start_degrees,
                                float end_degrees,
                                float start_time,
                                float end_time);
  virtual ~InterpolatedAxisAngleRotation();

 protected:
  virtual gfx::Transform InterpolateButDoNotCompose(float t) const OVERRIDE;

 private:
  gfx::Vector3dF axis_;
  const float start_degrees_;
  const float end_degrees_;

  DISALLOW_COPY_AND_ASSIGN(InterpolatedAxisAngleRotation);
};

///////////////////////////////////////////////////////////////////////////////
// class InterpolatedScale
//
// Represents an animated scale.
//
///////////////////////////////////////////////////////////////////////////////
class GFX_EXPORT InterpolatedScale : public InterpolatedTransform {
 public:
  InterpolatedScale(float start_scale, float end_scale);
  InterpolatedScale(float start_scale, float end_scale,
                    float start_time, float end_time);
  InterpolatedScale(const gfx::Point3F& start_scale,
                    const gfx::Point3F& end_scale);
  InterpolatedScale(const gfx::Point3F& start_scale,
                    const gfx::Point3F& end_scale,
                    float start_time,
                    float end_time);
  virtual ~InterpolatedScale();

 protected:
  virtual gfx::Transform InterpolateButDoNotCompose(float t) const OVERRIDE;

 private:
  const gfx::Point3F start_scale_;
  const gfx::Point3F end_scale_;

  DISALLOW_COPY_AND_ASSIGN(InterpolatedScale);
};

class GFX_EXPORT InterpolatedTranslation : public InterpolatedTransform {
 public:
  InterpolatedTranslation(const gfx::Point& start_pos,
                          const gfx::Point& end_pos);
  InterpolatedTranslation(const gfx::Point& start_pos,
                          const gfx::Point& end_pos,
                          float start_time,
                          float end_time);
  virtual ~InterpolatedTranslation();

 protected:
  virtual gfx::Transform InterpolateButDoNotCompose(float t) const OVERRIDE;

 private:
  const gfx::Point start_pos_;
  const gfx::Point end_pos_;

  DISALLOW_COPY_AND_ASSIGN(InterpolatedTranslation);
};

///////////////////////////////////////////////////////////////////////////////
// class InterpolatedConstantTransform
//
// Represents a transform that is constant over time. This is only useful when
// composed with other interpolated transforms.
//
// See InterpolatedTransformAboutPivot for an example of its usage.
//
///////////////////////////////////////////////////////////////////////////////
class GFX_EXPORT InterpolatedConstantTransform : public InterpolatedTransform {
 public:
  explicit InterpolatedConstantTransform(const gfx::Transform& transform);
  virtual ~InterpolatedConstantTransform();

 protected:
  virtual gfx::Transform InterpolateButDoNotCompose(float t) const OVERRIDE;

 private:
  const gfx::Transform transform_;

  DISALLOW_COPY_AND_ASSIGN(InterpolatedConstantTransform);
};

///////////////////////////////////////////////////////////////////////////////
// class InterpolatedTransformAboutPivot
//
// Represents an animated transform with a transformed origin. Essentially,
// at each time, t, the interpolated transform is created by composing
// P * T * P^-1 where P is a constant transform to the new origin.
//
///////////////////////////////////////////////////////////////////////////////
class GFX_EXPORT InterpolatedTransformAboutPivot
    : public InterpolatedTransform {
 public:
  // Takes ownership of the passed transform.
  InterpolatedTransformAboutPivot(const gfx::Point& pivot,
                                  InterpolatedTransform* transform);

  // Takes ownership of the passed transform.
  InterpolatedTransformAboutPivot(const gfx::Point& pivot,
                                  InterpolatedTransform* transform,
                                  float start_time,
                                  float end_time);
  virtual ~InterpolatedTransformAboutPivot();

 protected:
  virtual gfx::Transform InterpolateButDoNotCompose(float t) const OVERRIDE;

 private:
  void Init(const gfx::Point& pivot, InterpolatedTransform* transform);

  scoped_ptr<InterpolatedTransform> transform_;

  DISALLOW_COPY_AND_ASSIGN(InterpolatedTransformAboutPivot);
};

class GFX_EXPORT InterpolatedMatrixTransform : public InterpolatedTransform {
 public:
  InterpolatedMatrixTransform(const gfx::Transform& start_transform,
                              const gfx::Transform& end_transform);

  InterpolatedMatrixTransform(const gfx::Transform& start_transform,
                              const gfx::Transform& end_transform,
                              float start_time,
                              float end_time);

  virtual ~InterpolatedMatrixTransform();

 protected:
  virtual gfx::Transform InterpolateButDoNotCompose(float t) const OVERRIDE;

 private:
  void Init(const gfx::Transform& start_transform,
            const gfx::Transform& end_transform);

  gfx::DecomposedTransform start_decomp_;
  gfx::DecomposedTransform end_decomp_;
};

} // namespace ui

#endif  // UI_GFX_INTERPOLATED_TRANSFORM_H_
