/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// Modified from the Chromium original here:
// src/media/base/sinc_resampler.h

#ifndef WEBRTC_COMMON_AUDIO_RESAMPLER_SINC_RESAMPLER_H_
#define WEBRTC_COMMON_AUDIO_RESAMPLER_SINC_RESAMPLER_H_

#include "webrtc/base/constructormagic.h"
#include "webrtc/system_wrappers/interface/aligned_malloc.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/test/testsupport/gtest_prod_util.h"
#include "webrtc/typedefs.h"

namespace webrtc {

// Callback class for providing more data into the resampler.  Expects |frames|
// of data to be rendered into |destination|; zero padded if not enough frames
// are available to satisfy the request.
class SincResamplerCallback {
 public:
  virtual ~SincResamplerCallback() {}
  virtual void Run(int frames, float* destination) = 0;
};

// SincResampler is a high-quality single-channel sample-rate converter.
class SincResampler {
 public:
  enum {
    // The kernel size can be adjusted for quality (higher is better) at the
    // expense of performance.  Must be a multiple of 32.
    // TODO(dalecurtis): Test performance to see if we can jack this up to 64+.
    kKernelSize = 32,

    // Default request size.  Affects how often and for how much SincResampler
    // calls back for input.  Must be greater than kKernelSize.
    kDefaultRequestSize = 512,

    // The kernel offset count is used for interpolation and is the number of
    // sub-sample kernel shifts.  Can be adjusted for quality (higher is better)
    // at the expense of allocating more memory.
    kKernelOffsetCount = 32,
    kKernelStorageSize = kKernelSize * (kKernelOffsetCount + 1),
  };

  // Constructs a SincResampler with the specified |read_cb|, which is used to
  // acquire audio data for resampling.  |io_sample_rate_ratio| is the ratio
  // of input / output sample rates.  |request_frames| controls the size in
  // frames of the buffer requested by each |read_cb| call.  The value must be
  // greater than kKernelSize.  Specify kDefaultRequestSize if there are no
  // request size constraints.
  SincResampler(double io_sample_rate_ratio,
                int request_frames,
                SincResamplerCallback* read_cb);
  virtual ~SincResampler();

  // Resample |frames| of data from |read_cb_| into |destination|.
  void Resample(int frames, float* destination);

  // The maximum size in frames that guarantees Resample() will only make a
  // single call to |read_cb_| for more data.
  int ChunkSize() const;

  int request_frames() const { return request_frames_; }

  // Flush all buffered data and reset internal indices.  Not thread safe, do
  // not call while Resample() is in progress.
  void Flush();

  // Update |io_sample_rate_ratio_|.  SetRatio() will cause a reconstruction of
  // the kernels used for resampling.  Not thread safe, do not call while
  // Resample() is in progress.
  //
  // TODO(ajm): Use this in PushSincResampler rather than reconstructing
  // SincResampler.  We would also need a way to update |request_frames_|.
  void SetRatio(double io_sample_rate_ratio);

  float* get_kernel_for_testing() { return kernel_storage_.get(); }

 private:
  FRIEND_TEST_ALL_PREFIXES(SincResamplerTest, Convolve);
  FRIEND_TEST_ALL_PREFIXES(SincResamplerTest, ConvolveBenchmark);

  void InitializeKernel();
  void UpdateRegions(bool second_load);

  // Selects runtime specific CPU features like SSE.  Must be called before
  // using SincResampler.
  // TODO(ajm): Currently managed by the class internally. See the note with
  // |convolve_proc_| below.
  void InitializeCPUSpecificFeatures();

  // Compute convolution of |k1| and |k2| over |input_ptr|, resultant sums are
  // linearly interpolated using |kernel_interpolation_factor|.  On x86 and ARM
  // the underlying implementation is chosen at run time.
  static float Convolve_C(const float* input_ptr, const float* k1,
                          const float* k2, double kernel_interpolation_factor);
#if defined(WEBRTC_ARCH_X86_FAMILY)
  static float Convolve_SSE(const float* input_ptr, const float* k1,
                            const float* k2,
                            double kernel_interpolation_factor);
#elif defined(WEBRTC_ARCH_ARM_V7)
  static float Convolve_NEON(const float* input_ptr, const float* k1,
                             const float* k2,
                             double kernel_interpolation_factor);
#endif

  // The ratio of input / output sample rates.
  double io_sample_rate_ratio_;

  // An index on the source input buffer with sub-sample precision.  It must be
  // double precision to avoid drift.
  double virtual_source_idx_;

  // The buffer is primed once at the very beginning of processing.
  bool buffer_primed_;

  // Source of data for resampling.
  SincResamplerCallback* read_cb_;

  // The size (in samples) to request from each |read_cb_| execution.
  const int request_frames_;

  // The number of source frames processed per pass.
  int block_size_;

  // The size (in samples) of the internal buffer used by the resampler.
  const int input_buffer_size_;

  // Contains kKernelOffsetCount kernels back-to-back, each of size kKernelSize.
  // The kernel offsets are sub-sample shifts of a windowed sinc shifted from
  // 0.0 to 1.0 sample.
  scoped_ptr<float[], AlignedFreeDeleter> kernel_storage_;
  scoped_ptr<float[], AlignedFreeDeleter> kernel_pre_sinc_storage_;
  scoped_ptr<float[], AlignedFreeDeleter> kernel_window_storage_;

  // Data from the source is copied into this buffer for each processing pass.
  scoped_ptr<float[], AlignedFreeDeleter> input_buffer_;

  // Stores the runtime selection of which Convolve function to use.
  // TODO(ajm): Move to using a global static which must only be initialized
  // once by the user. We're not doing this initially, because we don't have
  // e.g. a LazyInstance helper in webrtc.
#if defined(WEBRTC_CPU_DETECTION)
  typedef float (*ConvolveProc)(const float*, const float*, const float*,
                                double);
  ConvolveProc convolve_proc_;
#endif

  // Pointers to the various regions inside |input_buffer_|.  See the diagram at
  // the top of the .cc file for more information.
  float* r0_;
  float* const r1_;
  float* const r2_;
  float* r3_;
  float* r4_;

  DISALLOW_COPY_AND_ASSIGN(SincResampler);
};

}  // namespace webrtc

#endif  // WEBRTC_COMMON_AUDIO_RESAMPLER_SINC_RESAMPLER_H_
