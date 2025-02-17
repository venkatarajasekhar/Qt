// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/output/renderer.h"

namespace cc {

bool Renderer::HasAllocatedResourcesForTesting(RenderPass::Id id) const {
  return false;
}

bool Renderer::IsContextLost() {
  return false;
}

void Renderer::SetVisible(bool visible) {
  if (visible_ == visible)
    return;

  visible_ = visible;
  DidChangeVisibility();
}

RendererCapabilitiesImpl::RendererCapabilitiesImpl()
    : best_texture_format(RGBA_8888),
      allow_partial_texture_updates(false),
      max_texture_size(0),
      using_shared_memory_resources(false),
      using_partial_swap(false),
      using_egl_image(false),
      avoid_pow2_textures(false),
      using_map_image(false),
      using_discard_framebuffer(false),
      allow_rasterize_on_demand(false) {}

RendererCapabilitiesImpl::~RendererCapabilitiesImpl() {}

RendererCapabilities RendererCapabilitiesImpl::MainThreadCapabilities() const {
  return RendererCapabilities(best_texture_format,
                              allow_partial_texture_updates,
                              max_texture_size,
                              using_shared_memory_resources);
}

}  // namespace cc
