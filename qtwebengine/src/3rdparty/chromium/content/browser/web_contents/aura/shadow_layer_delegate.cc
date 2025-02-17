// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/aura/shadow_layer_delegate.h"

#include "base/bind.h"
#include "third_party/skia/include/effects/SkGradientShader.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/skia_util.h"

namespace {

const SkColor kShadowLightColor = SkColorSetARGB(0x0, 0, 0, 0);
const SkColor kShadowDarkColor = SkColorSetARGB(0x70, 0, 0, 0);
const int kShadowThick = 7;

}  // namespace

namespace content {

ShadowLayerDelegate::ShadowLayerDelegate(ui::Layer* shadow_for)
    : layer_(new ui::Layer(ui::LAYER_TEXTURED)) {
  layer_->set_delegate(this);
  layer_->SetBounds(gfx::Rect(-kShadowThick, 0, kShadowThick,
                              shadow_for->bounds().height()));
  layer_->SetFillsBoundsOpaquely(false);
  shadow_for->Add(layer_.get());
}

ShadowLayerDelegate::~ShadowLayerDelegate() {
}

void ShadowLayerDelegate::OnPaintLayer(gfx::Canvas* canvas) {
  SkPoint points[2];
  const SkColor kShadowColors[2] = { kShadowLightColor, kShadowDarkColor };

  points[0].iset(0, 0);
  points[1].iset(kShadowThick, 0);

  skia::RefPtr<SkShader> shader = skia::AdoptRef(
      SkGradientShader::CreateLinear(points, kShadowColors, NULL,
          arraysize(points), SkShader::kRepeat_TileMode));

  gfx::Rect paint_rect = gfx::Rect(0, 0, kShadowThick,
                                   layer_->bounds().height());
  SkPaint paint;
  paint.setShader(shader.get());
  canvas->sk_canvas()->drawRect(gfx::RectToSkRect(paint_rect), paint);
}

void ShadowLayerDelegate::OnDeviceScaleFactorChanged(float scale_factor) {
}

base::Closure ShadowLayerDelegate::PrepareForLayerBoundsChange() {
  return base::Bind(&base::DoNothing);
}

}  // namespace content
