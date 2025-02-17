// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/layer_tree_host.h"

#include <sstream>

#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "cc/debug/lap_timer.h"
#include "cc/layers/content_layer.h"
#include "cc/layers/nine_patch_layer.h"
#include "cc/layers/solid_color_layer.h"
#include "cc/layers/texture_layer.h"
#include "cc/resources/texture_mailbox.h"
#include "cc/test/fake_content_layer_client.h"
#include "cc/test/layer_tree_json_parser.h"
#include "cc/test/layer_tree_test.h"
#include "cc/test/paths.h"
#include "cc/trees/layer_tree_impl.h"
#include "testing/perf/perf_test.h"

namespace cc {
namespace {

static const int kTimeLimitMillis = 2000;
static const int kWarmupRuns = 5;
static const int kTimeCheckInterval = 10;

class LayerTreeHostPerfTest : public LayerTreeTest {
 public:
  LayerTreeHostPerfTest()
      : draw_timer_(kWarmupRuns,
                    base::TimeDelta::FromMilliseconds(kTimeLimitMillis),
                    kTimeCheckInterval),
        commit_timer_(0, base::TimeDelta(), 1),
        full_damage_each_frame_(false),
        animation_driven_drawing_(false),
        measure_commit_cost_(false) {
    fake_content_layer_client_.set_paint_all_opaque(true);
  }

  virtual void InitializeSettings(LayerTreeSettings* settings) OVERRIDE {
    settings->throttle_frame_production = false;
  }

  virtual void BeginTest() OVERRIDE {
    BuildTree();
    PostSetNeedsCommitToMainThread();
  }

  virtual void Animate(base::TimeTicks monotonic_time) OVERRIDE {
    if (animation_driven_drawing_ && !TestEnded()) {
      layer_tree_host()->SetNeedsAnimate();
      layer_tree_host()->SetNextCommitForcesRedraw();
    }
  }

  virtual void BeginCommitOnThread(LayerTreeHostImpl* host_impl) OVERRIDE {
    if (measure_commit_cost_)
      commit_timer_.Start();
  }

  virtual void CommitCompleteOnThread(LayerTreeHostImpl* host_impl) OVERRIDE {
    if (measure_commit_cost_ && draw_timer_.IsWarmedUp()) {
      commit_timer_.NextLap();
    }
  }

  virtual void DrawLayersOnThread(LayerTreeHostImpl* impl) OVERRIDE {
    if (TestEnded() || CleanUpStarted())
      return;
    draw_timer_.NextLap();
    if (draw_timer_.HasTimeLimitExpired()) {
      CleanUpAndEndTest(impl);
      return;
    }
    if (!animation_driven_drawing_)
      impl->SetNeedsRedraw();
    if (full_damage_each_frame_)
      impl->SetFullRootLayerDamage();
  }

  virtual void CleanUpAndEndTest(LayerTreeHostImpl* host_impl) { EndTest(); }

  virtual bool CleanUpStarted() { return false; }

  virtual void BuildTree() {}

  virtual void AfterTest() OVERRIDE {
    CHECK(!test_name_.empty()) << "Must SetTestName() before AfterTest().";
    perf_test::PrintResult("layer_tree_host_frame_time", "", test_name_,
                           1000 * draw_timer_.MsPerLap(), "us", true);
    if (measure_commit_cost_) {
      perf_test::PrintResult("layer_tree_host_commit_time", "", test_name_,
                             1000 * commit_timer_.MsPerLap(), "us", true);
    }
  }

 protected:
  LapTimer draw_timer_;
  LapTimer commit_timer_;

  std::string test_name_;
  FakeContentLayerClient fake_content_layer_client_;
  bool full_damage_each_frame_;
  bool animation_driven_drawing_;

  bool measure_commit_cost_;
};


class LayerTreeHostPerfTestJsonReader : public LayerTreeHostPerfTest {
 public:
  LayerTreeHostPerfTestJsonReader()
      : LayerTreeHostPerfTest() {
  }

  void SetTestName(const std::string& name) {
    test_name_ = name;
  }

  void ReadTestFile(const std::string& name) {
    base::FilePath test_data_dir;
    ASSERT_TRUE(PathService::Get(CCPaths::DIR_TEST_DATA, &test_data_dir));
    base::FilePath json_file = test_data_dir.AppendASCII(name + ".json");
    ASSERT_TRUE(base::ReadFileToString(json_file, &json_));
  }

  virtual void BuildTree() OVERRIDE {
    gfx::Size viewport = gfx::Size(720, 1038);
    layer_tree_host()->SetViewportSize(viewport);
    scoped_refptr<Layer> root = ParseTreeFromJson(json_,
                                                  &fake_content_layer_client_);
    ASSERT_TRUE(root.get());
    layer_tree_host()->SetRootLayer(root);
  }

 private:
  std::string json_;
};

// Simulates a tab switcher scene with two stacks of 10 tabs each.
TEST_F(LayerTreeHostPerfTestJsonReader, TenTenSingleThread) {
  SetTestName("10_10_single_thread");
  ReadTestFile("10_10_layer_tree");
  RunTest(false, false, false);
}

TEST_F(LayerTreeHostPerfTestJsonReader, TenTenThreadedImplSide) {
  SetTestName("10_10_threaded_impl_side");
  ReadTestFile("10_10_layer_tree");
  RunTestWithImplSidePainting();
}

// Simulates a tab switcher scene with two stacks of 10 tabs each.
TEST_F(LayerTreeHostPerfTestJsonReader,
       TenTenSingleThread_FullDamageEachFrame) {
  full_damage_each_frame_ = true;
  SetTestName("10_10_single_thread_full_damage_each_frame");
  ReadTestFile("10_10_layer_tree");
  RunTest(false, false, false);
}

TEST_F(LayerTreeHostPerfTestJsonReader,
       TenTenThreadedImplSide_FullDamageEachFrame) {
  full_damage_each_frame_ = true;
  SetTestName("10_10_threaded_impl_side_full_damage_each_frame");
  ReadTestFile("10_10_layer_tree");
  RunTestWithImplSidePainting();
}

// Invalidates a leaf layer in the tree on the main thread after every commit.
class LayerTreeHostPerfTestLeafInvalidates
    : public LayerTreeHostPerfTestJsonReader {
 public:
  virtual void BuildTree() OVERRIDE {
    LayerTreeHostPerfTestJsonReader::BuildTree();

    // Find a leaf layer.
    for (layer_to_invalidate_ = layer_tree_host()->root_layer();
         layer_to_invalidate_->children().size();
         layer_to_invalidate_ = layer_to_invalidate_->children()[0]) {}
  }

  virtual void DidCommitAndDrawFrame() OVERRIDE {
    if (TestEnded())
      return;

    static bool flip = true;
    layer_to_invalidate_->SetOpacity(flip ? 1.f : 0.5f);
    flip = !flip;
  }

 protected:
  Layer* layer_to_invalidate_;
};

// Simulates a tab switcher scene with two stacks of 10 tabs each. Invalidate a
// property on a leaf layer in the tree every commit.
TEST_F(LayerTreeHostPerfTestLeafInvalidates, TenTenSingleThread) {
  SetTestName("10_10_single_thread_leaf_invalidates");
  ReadTestFile("10_10_layer_tree");
  RunTest(false, false, false);
}

TEST_F(LayerTreeHostPerfTestLeafInvalidates, TenTenThreadedImplSide) {
  SetTestName("10_10_threaded_impl_side_leaf_invalidates");
  ReadTestFile("10_10_layer_tree");
  RunTestWithImplSidePainting();
}

// Simulates main-thread scrolling on each frame.
class ScrollingLayerTreePerfTest : public LayerTreeHostPerfTestJsonReader {
 public:
  ScrollingLayerTreePerfTest()
      : LayerTreeHostPerfTestJsonReader() {
  }

  virtual void BuildTree() OVERRIDE {
    LayerTreeHostPerfTestJsonReader::BuildTree();
    scrollable_ = layer_tree_host()->root_layer()->children()[1];
    ASSERT_TRUE(scrollable_.get());
  }

  virtual void Layout() OVERRIDE {
    static const gfx::Vector2d delta = gfx::Vector2d(0, 10);
    scrollable_->SetScrollOffset(scrollable_->scroll_offset() + delta);
  }

 private:
  scoped_refptr<Layer> scrollable_;
};

TEST_F(ScrollingLayerTreePerfTest, LongScrollablePageSingleThread) {
  SetTestName("long_scrollable_page");
  ReadTestFile("long_scrollable_page");
  RunTest(false, false, false);
}

TEST_F(ScrollingLayerTreePerfTest, LongScrollablePageThreadedImplSide) {
  SetTestName("long_scrollable_page_threaded_impl_side");
  ReadTestFile("long_scrollable_page");
  RunTestWithImplSidePainting();
}

static void EmptyReleaseCallback(uint32 sync_point, bool lost_resource) {}

// Simulates main-thread scrolling on each frame.
class BrowserCompositorInvalidateLayerTreePerfTest
    : public LayerTreeHostPerfTestJsonReader {
 public:
  BrowserCompositorInvalidateLayerTreePerfTest()
      : LayerTreeHostPerfTestJsonReader(),
        next_sync_point_(1),
        clean_up_started_(false) {}

  virtual void BuildTree() OVERRIDE {
    LayerTreeHostPerfTestJsonReader::BuildTree();
    tab_contents_ =
        static_cast<TextureLayer*>(
            layer_tree_host()->root_layer()->children()[0]->
                                             children()[0]->
                                             children()[0]->
                                             children()[0].get());
    ASSERT_TRUE(tab_contents_.get());
  }

  virtual void WillCommit() OVERRIDE {
    if (CleanUpStarted())
      return;
    gpu::Mailbox gpu_mailbox;
    std::ostringstream name_stream;
    name_stream << "name" << next_sync_point_;
    gpu_mailbox.SetName(
        reinterpret_cast<const int8*>(name_stream.str().c_str()));
    scoped_ptr<SingleReleaseCallback> callback = SingleReleaseCallback::Create(
        base::Bind(&EmptyReleaseCallback));
    TextureMailbox mailbox(gpu_mailbox, GL_TEXTURE_2D, next_sync_point_);
    next_sync_point_++;

    tab_contents_->SetTextureMailbox(mailbox, callback.Pass());
  }

  virtual void DidCommit() OVERRIDE {
    if (CleanUpStarted())
      return;
    layer_tree_host()->SetNeedsCommit();
  }

  virtual void CleanUpAndEndTest(LayerTreeHostImpl* host_impl) OVERRIDE {
    clean_up_started_ = true;
    MainThreadTaskRunner()->PostTask(
        FROM_HERE,
        base::Bind(&BrowserCompositorInvalidateLayerTreePerfTest::
                        CleanUpAndEndTestOnMainThread,
                   base::Unretained(this)));
  }

  void CleanUpAndEndTestOnMainThread() {
    tab_contents_->SetTextureMailbox(TextureMailbox(),
                                     scoped_ptr<SingleReleaseCallback>());
    EndTest();
  }

  virtual bool CleanUpStarted() OVERRIDE { return clean_up_started_; }

 private:
  scoped_refptr<TextureLayer> tab_contents_;
  unsigned next_sync_point_;
  bool clean_up_started_;
};

TEST_F(BrowserCompositorInvalidateLayerTreePerfTest, DenseBrowserUI) {
  measure_commit_cost_ = true;
  SetTestName("dense_layer_tree");
  ReadTestFile("dense_layer_tree");
  RunTestWithImplSidePainting();
}

// Simulates a page with several large, transformed and animated layers.
TEST_F(LayerTreeHostPerfTestJsonReader, HeavyPageThreadedImplSide) {
  animation_driven_drawing_ = true;
  measure_commit_cost_ = true;
  SetTestName("heavy_page");
  ReadTestFile("heavy_layer_tree");
  RunTestWithImplSidePainting();
}

}  // namespace
}  // namespace cc
