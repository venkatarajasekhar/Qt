// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/output/software_renderer.h"

#include "base/run_loop.h"
#include "cc/layers/quad_sink.h"
#include "cc/output/compositor_frame_metadata.h"
#include "cc/output/copy_output_request.h"
#include "cc/output/copy_output_result.h"
#include "cc/output/software_output_device.h"
#include "cc/quads/render_pass.h"
#include "cc/quads/render_pass_draw_quad.h"
#include "cc/quads/solid_color_draw_quad.h"
#include "cc/quads/tile_draw_quad.h"
#include "cc/test/animation_test_common.h"
#include "cc/test/fake_output_surface.h"
#include "cc/test/fake_output_surface_client.h"
#include "cc/test/geometry_test_utils.h"
#include "cc/test/render_pass_test_common.h"
#include "cc/test/render_pass_test_utils.h"
#include "cc/test/test_shared_bitmap_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkCanvas.h"

namespace cc {
namespace {

class SoftwareRendererTest : public testing::Test, public RendererClient {
 public:
  void InitializeRenderer(
      scoped_ptr<SoftwareOutputDevice> software_output_device) {
    output_surface_ = FakeOutputSurface::CreateSoftware(
        software_output_device.Pass());
    CHECK(output_surface_->BindToClient(&output_surface_client_));

    shared_bitmap_manager_.reset(new TestSharedBitmapManager());
    resource_provider_ = ResourceProvider::Create(
        output_surface_.get(), shared_bitmap_manager_.get(), 0, false, 1,
        false);
    renderer_ = SoftwareRenderer::Create(
        this, &settings_, output_surface_.get(), resource_provider());
  }

  ResourceProvider* resource_provider() const {
    return resource_provider_.get();
  }

  SoftwareRenderer* renderer() const { return renderer_.get(); }

  // RendererClient implementation.
  virtual void SetFullRootLayerDamage() OVERRIDE {}
  virtual void RunOnDemandRasterTask(Task* on_demand_raster_task) OVERRIDE {}

  scoped_ptr<SkBitmap> DrawAndCopyOutput(RenderPassList* list,
                                         float device_scale_factor,
                                         gfx::Rect device_viewport_rect) {
    scoped_ptr<SkBitmap> bitmap_result;
    base::RunLoop loop;

    list->back()->copy_requests.push_back(
        CopyOutputRequest::CreateBitmapRequest(
            base::Bind(&SoftwareRendererTest::SaveBitmapResult,
                       base::Unretained(&bitmap_result),
                       loop.QuitClosure())));

    renderer()->DrawFrame(list,
                          device_scale_factor,
                          device_viewport_rect,
                          device_viewport_rect,
                          false);
    loop.Run();
    return bitmap_result.Pass();
  }

  static void SaveBitmapResult(scoped_ptr<SkBitmap>* bitmap_result,
                               const base::Closure& quit_closure,
                               scoped_ptr<CopyOutputResult> result) {
    DCHECK(result->HasBitmap());
    *bitmap_result = result->TakeBitmap();
    quit_closure.Run();
  }

 protected:
  LayerTreeSettings settings_;
  FakeOutputSurfaceClient output_surface_client_;
  scoped_ptr<FakeOutputSurface> output_surface_;
  scoped_ptr<SharedBitmapManager> shared_bitmap_manager_;
  scoped_ptr<ResourceProvider> resource_provider_;
  scoped_ptr<SoftwareRenderer> renderer_;
};

TEST_F(SoftwareRendererTest, SolidColorQuad) {
  gfx::Size outer_size(100, 100);
  gfx::Size inner_size(98, 98);
  gfx::Rect outer_rect(outer_size);
  gfx::Rect inner_rect(gfx::Point(1, 1), inner_size);
  gfx::Rect visible_rect(gfx::Point(1, 2), gfx::Size(98, 97));

  InitializeRenderer(make_scoped_ptr(new SoftwareOutputDevice));

  RenderPass::Id root_render_pass_id = RenderPass::Id(1, 1);
  scoped_ptr<TestRenderPass> root_render_pass = TestRenderPass::Create();
  root_render_pass->SetNew(
      root_render_pass_id, outer_rect, outer_rect, gfx::Transform());
  SharedQuadState* shared_quad_state =
      root_render_pass->CreateAndAppendSharedQuadState();
  shared_quad_state->SetAll(gfx::Transform(),
                            outer_size,
                            outer_rect,
                            outer_rect,
                            false,
                            1.0,
                            SkXfermode::kSrcOver_Mode,
                            0);
  scoped_ptr<SolidColorDrawQuad> outer_quad = SolidColorDrawQuad::Create();
  outer_quad->SetNew(
      shared_quad_state, outer_rect, outer_rect, SK_ColorYELLOW, false);
  scoped_ptr<SolidColorDrawQuad> inner_quad = SolidColorDrawQuad::Create();
  inner_quad->SetNew(
      shared_quad_state, inner_rect, inner_rect, SK_ColorCYAN, false);
  inner_quad->visible_rect = visible_rect;
  root_render_pass->AppendQuad(inner_quad.PassAs<DrawQuad>());
  root_render_pass->AppendQuad(outer_quad.PassAs<DrawQuad>());

  RenderPassList list;
  list.push_back(root_render_pass.PassAs<RenderPass>());

  float device_scale_factor = 1.f;
  gfx::Rect device_viewport_rect(outer_size);
  scoped_ptr<SkBitmap> output =
      DrawAndCopyOutput(&list, device_scale_factor, device_viewport_rect);
  EXPECT_EQ(outer_rect.width(), output->info().fWidth);
  EXPECT_EQ(outer_rect.width(), output->info().fHeight);

  EXPECT_EQ(SK_ColorYELLOW, output->getColor(0, 0));
  EXPECT_EQ(SK_ColorYELLOW,
            output->getColor(outer_size.width() - 1, outer_size.height() - 1));
  EXPECT_EQ(SK_ColorYELLOW, output->getColor(1, 1));
  EXPECT_EQ(SK_ColorCYAN, output->getColor(1, 2));
  EXPECT_EQ(SK_ColorCYAN,
            output->getColor(inner_size.width() - 1, inner_size.height() - 1));
}

TEST_F(SoftwareRendererTest, TileQuad) {
  gfx::Size outer_size(100, 100);
  gfx::Size inner_size(98, 98);
  gfx::Rect outer_rect(outer_size);
  gfx::Rect inner_rect(gfx::Point(1, 1), inner_size);
  InitializeRenderer(make_scoped_ptr(new SoftwareOutputDevice));

  ResourceProvider::ResourceId resource_yellow =
      resource_provider()->CreateResource(outer_size,
                                          GL_CLAMP_TO_EDGE,
                                          ResourceProvider::TextureUsageAny,
                                          RGBA_8888);
  ResourceProvider::ResourceId resource_cyan =
      resource_provider()->CreateResource(inner_size,
                                          GL_CLAMP_TO_EDGE,
                                          ResourceProvider::TextureUsageAny,
                                          RGBA_8888);

  SkBitmap yellow_tile;
  yellow_tile.setConfig(
      SkBitmap::kARGB_8888_Config, outer_size.width(), outer_size.height());
  yellow_tile.allocPixels();
  yellow_tile.eraseColor(SK_ColorYELLOW);

  SkBitmap cyan_tile;
  cyan_tile.setConfig(
      SkBitmap::kARGB_8888_Config, inner_size.width(), inner_size.height());
  cyan_tile.allocPixels();
  cyan_tile.eraseColor(SK_ColorCYAN);

  resource_provider()->SetPixels(
      resource_yellow,
      static_cast<uint8_t*>(yellow_tile.getPixels()),
      gfx::Rect(outer_size),
      gfx::Rect(outer_size),
      gfx::Vector2d());
  resource_provider()->SetPixels(resource_cyan,
                                 static_cast<uint8_t*>(cyan_tile.getPixels()),
                                 gfx::Rect(inner_size),
                                 gfx::Rect(inner_size),
                                 gfx::Vector2d());

  gfx::Rect root_rect = outer_rect;

  RenderPass::Id root_render_pass_id = RenderPass::Id(1, 1);
  scoped_ptr<TestRenderPass> root_render_pass = TestRenderPass::Create();
  root_render_pass->SetNew(
      root_render_pass_id, root_rect, root_rect, gfx::Transform());
  SharedQuadState* shared_quad_state =
      root_render_pass->CreateAndAppendSharedQuadState();
  shared_quad_state->SetAll(gfx::Transform(),
                            outer_size,
                            outer_rect,
                            outer_rect,
                            false,
                            1.0,
                            SkXfermode::kSrcOver_Mode,
                            0);
  scoped_ptr<TileDrawQuad> outer_quad = TileDrawQuad::Create();
  outer_quad->SetNew(shared_quad_state,
                     outer_rect,
                     outer_rect,
                     outer_rect,
                     resource_yellow,
                     gfx::RectF(outer_size),
                     outer_size,
                     false);
  scoped_ptr<TileDrawQuad> inner_quad = TileDrawQuad::Create();
  inner_quad->SetNew(shared_quad_state,
                     inner_rect,
                     inner_rect,
                     inner_rect,
                     resource_cyan,
                     gfx::RectF(inner_size),
                     inner_size,
                     false);
  root_render_pass->AppendQuad(inner_quad.PassAs<DrawQuad>());
  root_render_pass->AppendQuad(outer_quad.PassAs<DrawQuad>());

  RenderPassList list;
  list.push_back(root_render_pass.PassAs<RenderPass>());

  float device_scale_factor = 1.f;
  gfx::Rect device_viewport_rect(outer_size);
  scoped_ptr<SkBitmap> output =
      DrawAndCopyOutput(&list, device_scale_factor, device_viewport_rect);
  EXPECT_EQ(outer_rect.width(), output->info().fWidth);
  EXPECT_EQ(outer_rect.width(), output->info().fHeight);

  EXPECT_EQ(SK_ColorYELLOW, output->getColor(0, 0));
  EXPECT_EQ(SK_ColorYELLOW,
            output->getColor(outer_size.width() - 1, outer_size.height() - 1));
  EXPECT_EQ(SK_ColorCYAN, output->getColor(1, 1));
  EXPECT_EQ(SK_ColorCYAN,
            output->getColor(inner_size.width() - 1, inner_size.height() - 1));
}

TEST_F(SoftwareRendererTest, TileQuadVisibleRect) {
  gfx::Size tile_size(100, 100);
  gfx::Rect tile_rect(tile_size);
  gfx::Rect visible_rect = tile_rect;
  visible_rect.Inset(1, 2, 3, 4);
  InitializeRenderer(make_scoped_ptr(new SoftwareOutputDevice));

  ResourceProvider::ResourceId resource_cyan =
      resource_provider()->CreateResource(tile_size,
                                          GL_CLAMP_TO_EDGE,
                                          ResourceProvider::TextureUsageAny,
                                          RGBA_8888);

  SkBitmap cyan_tile;  // The lowest five rows are yellow.
  cyan_tile.setConfig(
      SkBitmap::kARGB_8888_Config, tile_size.width(), tile_size.height());
  cyan_tile.allocPixels();
  cyan_tile.eraseColor(SK_ColorCYAN);
  cyan_tile.eraseArea(
      SkIRect::MakeLTRB(
          0, visible_rect.bottom() - 1, tile_rect.width(), tile_rect.bottom()),
      SK_ColorYELLOW);

  resource_provider()->SetPixels(resource_cyan,
                                 static_cast<uint8_t*>(cyan_tile.getPixels()),
                                 gfx::Rect(tile_size),
                                 gfx::Rect(tile_size),
                                 gfx::Vector2d());

  gfx::Rect root_rect(tile_size);

  RenderPass::Id root_render_pass_id = RenderPass::Id(1, 1);
  scoped_ptr<TestRenderPass> root_render_pass = TestRenderPass::Create();
  root_render_pass->SetNew(
      root_render_pass_id, root_rect, root_rect, gfx::Transform());
  SharedQuadState* shared_quad_state =
      root_render_pass->CreateAndAppendSharedQuadState();
  shared_quad_state->SetAll(gfx::Transform(),
                            tile_size,
                            tile_rect,
                            tile_rect,
                            false,
                            1.0,
                            SkXfermode::kSrcOver_Mode,
                            0);
  scoped_ptr<TileDrawQuad> quad = TileDrawQuad::Create();
  quad->SetNew(shared_quad_state,
               tile_rect,
               tile_rect,
               tile_rect,
               resource_cyan,
               gfx::RectF(tile_size),
               tile_size,
               false);
  quad->visible_rect = visible_rect;
  root_render_pass->AppendQuad(quad.PassAs<DrawQuad>());

  RenderPassList list;
  list.push_back(root_render_pass.PassAs<RenderPass>());

  float device_scale_factor = 1.f;
  gfx::Rect device_viewport_rect(tile_size);
  scoped_ptr<SkBitmap> output =
      DrawAndCopyOutput(&list, device_scale_factor, device_viewport_rect);
  EXPECT_EQ(tile_rect.width(), output->info().fWidth);
  EXPECT_EQ(tile_rect.width(), output->info().fHeight);

  // Check portion of tile not in visible rect isn't drawn.
  const unsigned int kTransparent = SK_ColorTRANSPARENT;
  EXPECT_EQ(kTransparent, output->getColor(0, 0));
  EXPECT_EQ(kTransparent,
            output->getColor(tile_rect.width() - 1, tile_rect.height() - 1));
  EXPECT_EQ(kTransparent,
            output->getColor(visible_rect.x() - 1, visible_rect.y() - 1));
  EXPECT_EQ(kTransparent,
            output->getColor(visible_rect.right(), visible_rect.bottom()));
  // Ensure visible part is drawn correctly.
  EXPECT_EQ(SK_ColorCYAN, output->getColor(visible_rect.x(), visible_rect.y()));
  EXPECT_EQ(
      SK_ColorCYAN,
      output->getColor(visible_rect.right() - 2, visible_rect.bottom() - 2));
  // Ensure last visible line is correct.
  EXPECT_EQ(
      SK_ColorYELLOW,
      output->getColor(visible_rect.right() - 1, visible_rect.bottom() - 1));
}

TEST_F(SoftwareRendererTest, ShouldClearRootRenderPass) {
  float device_scale_factor = 1.f;
  gfx::Rect device_viewport_rect(0, 0, 100, 100);

  settings_.should_clear_root_render_pass = false;
  InitializeRenderer(make_scoped_ptr(new SoftwareOutputDevice));

  RenderPassList list;

  // Draw a fullscreen green quad in a first frame.
  RenderPass::Id root_clear_pass_id(1, 0);
  TestRenderPass* root_clear_pass = AddRenderPass(
      &list, root_clear_pass_id, device_viewport_rect, gfx::Transform());
  AddQuad(root_clear_pass, device_viewport_rect, SK_ColorGREEN);

  renderer()->DecideRenderPassAllocationsForFrame(list);

  scoped_ptr<SkBitmap> output =
      DrawAndCopyOutput(&list, device_scale_factor, device_viewport_rect);
  EXPECT_EQ(device_viewport_rect.width(), output->info().fWidth);
  EXPECT_EQ(device_viewport_rect.width(), output->info().fHeight);

  EXPECT_EQ(SK_ColorGREEN, output->getColor(0, 0));
  EXPECT_EQ(SK_ColorGREEN,
            output->getColor(device_viewport_rect.width() - 1,
                             device_viewport_rect.height() - 1));

  list.clear();

  // Draw a smaller magenta rect without filling the viewport in a separate
  // frame.
  gfx::Rect smaller_rect(20, 20, 60, 60);

  RenderPass::Id root_smaller_pass_id(2, 0);
  TestRenderPass* root_smaller_pass = AddRenderPass(
      &list, root_smaller_pass_id, device_viewport_rect, gfx::Transform());
  AddQuad(root_smaller_pass, smaller_rect, SK_ColorMAGENTA);

  renderer()->DecideRenderPassAllocationsForFrame(list);

  output = DrawAndCopyOutput(&list, device_scale_factor, device_viewport_rect);
  EXPECT_EQ(device_viewport_rect.width(), output->info().fWidth);
  EXPECT_EQ(device_viewport_rect.width(), output->info().fHeight);

  // If we didn't clear, the borders should still be green.
  EXPECT_EQ(SK_ColorGREEN, output->getColor(0, 0));
  EXPECT_EQ(SK_ColorGREEN,
            output->getColor(device_viewport_rect.width() - 1,
                             device_viewport_rect.height() - 1));

  EXPECT_EQ(SK_ColorMAGENTA,
            output->getColor(smaller_rect.x(), smaller_rect.y()));
  EXPECT_EQ(
      SK_ColorMAGENTA,
      output->getColor(smaller_rect.right() - 1, smaller_rect.bottom() - 1));
}

TEST_F(SoftwareRendererTest, RenderPassVisibleRect) {
  float device_scale_factor = 1.f;
  gfx::Rect device_viewport_rect(0, 0, 100, 100);
  InitializeRenderer(make_scoped_ptr(new SoftwareOutputDevice));

  RenderPassList list;

  // Pass drawn as inner quad is magenta.
  gfx::Rect smaller_rect(20, 20, 60, 60);
  RenderPass::Id smaller_pass_id(2, 1);
  TestRenderPass* smaller_pass =
      AddRenderPass(&list, smaller_pass_id, smaller_rect, gfx::Transform());
  AddQuad(smaller_pass, smaller_rect, SK_ColorMAGENTA);

  // Root pass is green.
  RenderPass::Id root_clear_pass_id(1, 0);
  TestRenderPass* root_clear_pass = AddRenderPass(
      &list, root_clear_pass_id, device_viewport_rect, gfx::Transform());
  AddRenderPassQuad(root_clear_pass, smaller_pass);
  AddQuad(root_clear_pass, device_viewport_rect, SK_ColorGREEN);

  // Interior pass quad has smaller visible rect.
  gfx::Rect interior_visible_rect(30, 30, 40, 40);
  root_clear_pass->quad_list[0]->visible_rect = interior_visible_rect;

  renderer()->DecideRenderPassAllocationsForFrame(list);

  scoped_ptr<SkBitmap> output =
      DrawAndCopyOutput(&list, device_scale_factor, device_viewport_rect);
  EXPECT_EQ(device_viewport_rect.width(), output->info().fWidth);
  EXPECT_EQ(device_viewport_rect.width(), output->info().fHeight);

  EXPECT_EQ(SK_ColorGREEN, output->getColor(0, 0));
  EXPECT_EQ(SK_ColorGREEN,
            output->getColor(device_viewport_rect.width() - 1,
                             device_viewport_rect.height() - 1));

  // Part outside visible rect should remain green.
  EXPECT_EQ(SK_ColorGREEN,
            output->getColor(smaller_rect.x(), smaller_rect.y()));
  EXPECT_EQ(
      SK_ColorGREEN,
      output->getColor(smaller_rect.right() - 1, smaller_rect.bottom() - 1));

  EXPECT_EQ(
      SK_ColorMAGENTA,
      output->getColor(interior_visible_rect.x(), interior_visible_rect.y()));
  EXPECT_EQ(SK_ColorMAGENTA,
            output->getColor(interior_visible_rect.right() - 1,
                             interior_visible_rect.bottom() - 1));
}

}  // namespace
}  // namespace cc
