// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_DEBUG_RASTERIZE_AND_RECORD_BENCHMARK_IMPL_H_
#define CC_DEBUG_RASTERIZE_AND_RECORD_BENCHMARK_IMPL_H_

#include <map>
#include <utility>
#include <vector>

#include "base/time/time.h"
#include "cc/debug/micro_benchmark_impl.h"
#include "cc/resources/task_graph_runner.h"

namespace cc {

class LayerTreeHostImpl;
class PictureLayerImpl;
class LayerImpl;
class RasterizeAndRecordBenchmarkImpl : public MicroBenchmarkImpl {
 public:
  explicit RasterizeAndRecordBenchmarkImpl(
      scoped_refptr<base::MessageLoopProxy> origin_loop,
      base::Value* value,
      const MicroBenchmarkImpl::DoneCallback& callback);
  virtual ~RasterizeAndRecordBenchmarkImpl();

  // Implements MicroBenchmark interface.
  virtual void DidCompleteCommit(LayerTreeHostImpl* host) OVERRIDE;
  virtual void RunOnLayer(PictureLayerImpl* layer) OVERRIDE;

 private:
  void Run(LayerImpl* layer);

  struct RasterizeResults {
    RasterizeResults();
    ~RasterizeResults();

    int pixels_rasterized;
    int pixels_rasterized_with_non_solid_color;
    int pixels_rasterized_as_opaque;
    base::TimeDelta total_best_time;
    int total_layers;
    int total_picture_layers;
    int total_picture_layers_with_no_content;
    int total_picture_layers_off_screen;
  };

  RasterizeResults rasterize_results_;
  int rasterize_repeat_count_;
  NamespaceToken task_namespace_;
};

}  // namespace cc

#endif  // CC_DEBUG_RASTERIZE_AND_RECORD_BENCHMARK_IMPL_H_
