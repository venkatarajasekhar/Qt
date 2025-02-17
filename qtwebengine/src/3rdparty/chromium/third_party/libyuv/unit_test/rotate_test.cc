/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>
#include <time.h>

#include "libyuv/cpu_id.h"
#include "libyuv/rotate.h"
#include "libyuv/row.h"
#include "../unit_test/unit_test.h"

namespace libyuv {

static void I420TestRotate(int src_width, int src_height,
                           int dst_width, int dst_height,
                           libyuv::RotationMode mode,
                           int benchmark_iterations) {
  if (src_width < 1) {
    src_width = 1;
  }
  if (src_height < 1) {
    src_height = 1;
  }
  if (dst_width < 1) {
    dst_width = 1;
  }
  if (dst_height < 1) {
    dst_height = 1;
  }
  int src_i420_y_size = src_width * src_height;
  int src_i420_uv_size = ((src_width + 1) / 2) * ((src_height + 1) / 2);
  int src_i420_size = src_i420_y_size + src_i420_uv_size * 2;
  align_buffer_64(src_i420, src_i420_size);
  for (int i = 0; i < src_i420_size; ++i) {
    src_i420[i] = random() & 0xff;
  }

  int dst_i420_y_size = dst_width * dst_height;
  int dst_i420_uv_size = ((dst_width + 1) / 2) * ((dst_height + 1) / 2);
  int dst_i420_size = dst_i420_y_size + dst_i420_uv_size * 2;
  align_buffer_64(dst_i420_c, dst_i420_size);
  align_buffer_64(dst_i420_opt, dst_i420_size);
  memset(dst_i420_c, 2, dst_i420_size);
  memset(dst_i420_opt, 3, dst_i420_size);

  MaskCpuFlags(0);  // Disable all CPU optimization.
  I420Rotate(src_i420, src_width,
             src_i420 + src_i420_y_size, (src_width + 1) / 2,
             src_i420 + src_i420_y_size + src_i420_uv_size, (src_width + 1) / 2,
             dst_i420_c, dst_width,
             dst_i420_c + dst_i420_y_size, (dst_width + 1) / 2,
             dst_i420_c + dst_i420_y_size + dst_i420_uv_size,
               (dst_width + 1) / 2,
             src_width, src_height, mode);

  MaskCpuFlags(-1);  // Enable all CPU optimization.
  for (int i = 0; i < benchmark_iterations; ++i) {
    I420Rotate(src_i420, src_width,
               src_i420 + src_i420_y_size, (src_width + 1) / 2,
               src_i420 + src_i420_y_size + src_i420_uv_size,
                 (src_width + 1) / 2,
               dst_i420_opt, dst_width,
               dst_i420_opt + dst_i420_y_size, (dst_width + 1) / 2,
               dst_i420_opt + dst_i420_y_size + dst_i420_uv_size,
                 (dst_width + 1) / 2,
               src_width, src_height, mode);
  }

  // Rotation should be exact.
  for (int i = 0; i < dst_i420_size; ++i) {
    EXPECT_EQ(dst_i420_c[i], dst_i420_opt[i]);
  }

  free_aligned_buffer_64(dst_i420_c);
  free_aligned_buffer_64(dst_i420_opt);
  free_aligned_buffer_64(src_i420);
}

TEST_F(libyuvTest, I420Rotate0) {
  I420TestRotate(benchmark_width_, benchmark_height_,
                 benchmark_width_, benchmark_height_,
                 kRotate0, benchmark_iterations_);
}

TEST_F(libyuvTest, I420Rotate90) {
  I420TestRotate(benchmark_width_, benchmark_height_,
                 benchmark_height_, benchmark_width_,
                 kRotate90, benchmark_iterations_);
}

TEST_F(libyuvTest, I420Rotate180) {
  I420TestRotate(benchmark_width_, benchmark_height_,
                 benchmark_width_, benchmark_height_,
                 kRotate180, benchmark_iterations_);
}

TEST_F(libyuvTest, I420Rotate270) {
  I420TestRotate(benchmark_width_, benchmark_height_,
                 benchmark_height_, benchmark_width_,
                 kRotate270, benchmark_iterations_);
}

TEST_F(libyuvTest, I420Rotate0_Odd) {
  I420TestRotate(benchmark_width_ - 3, benchmark_height_ - 1,
                 benchmark_width_ - 3, benchmark_height_ - 1,
                 kRotate0, benchmark_iterations_);
}

TEST_F(libyuvTest, I420Rotate90_Odd) {
  I420TestRotate(benchmark_width_ - 3, benchmark_height_ - 1,
                 benchmark_height_ - 1, benchmark_width_ - 3,
                 kRotate90, benchmark_iterations_);
}

TEST_F(libyuvTest, I420Rotate180_Odd) {
  I420TestRotate(benchmark_width_ - 3, benchmark_height_ - 1,
                 benchmark_width_ - 3, benchmark_height_ - 1,
                 kRotate180, benchmark_iterations_);
}

TEST_F(libyuvTest, I420Rotate270_Odd) {
  I420TestRotate(benchmark_width_ - 3, benchmark_height_ - 1,
                 benchmark_height_ - 1, benchmark_width_ - 3,
                 kRotate270, benchmark_iterations_);
}

static void NV12TestRotate(int src_width, int src_height,
                           int dst_width, int dst_height,
                           libyuv::RotationMode mode,
                           int benchmark_iterations) {
  if (src_width < 1) {
    src_width = 1;
  }
  if (src_height < 1) {
    src_height = 1;
  }
  if (dst_width < 1) {
    dst_width = 1;
  }
  if (dst_height < 1) {
    dst_height = 1;
  }
  int src_nv12_y_size = src_width * src_height;
  int src_nv12_uv_size = ((src_width + 1) / 2) * ((src_height + 1) / 2) * 2;
  int src_nv12_size = src_nv12_y_size + src_nv12_uv_size;
  align_buffer_64(src_nv12, src_nv12_size);
  for (int i = 0; i < src_nv12_size; ++i) {
    src_nv12[i] = random() & 0xff;
  }

  int dst_i420_y_size = dst_width * dst_height;
  int dst_i420_uv_size = ((dst_width + 1) / 2) * ((dst_height + 1) / 2);
  int dst_i420_size = dst_i420_y_size + dst_i420_uv_size * 2;
  align_buffer_64(dst_i420_c, dst_i420_size);
  align_buffer_64(dst_i420_opt, dst_i420_size);
  memset(dst_i420_c, 2, dst_i420_size);
  memset(dst_i420_opt, 3, dst_i420_size);

  MaskCpuFlags(0);  // Disable all CPU optimization.
  NV12ToI420Rotate(src_nv12, src_width,
                   src_nv12 + src_nv12_y_size, (src_width + 1) & ~1,
                   dst_i420_c, dst_width,
                   dst_i420_c + dst_i420_y_size, (dst_width + 1) / 2,
                   dst_i420_c + dst_i420_y_size + dst_i420_uv_size,
                     (dst_width + 1) / 2,
                   src_width, src_height, mode);

  MaskCpuFlags(-1);  // Enable all CPU optimization.
  for (int i = 0; i < benchmark_iterations; ++i) {
    NV12ToI420Rotate(src_nv12, src_width,
                     src_nv12 + src_nv12_y_size, (src_width + 1) & ~1,
                     dst_i420_opt, dst_width,
                     dst_i420_opt + dst_i420_y_size, (dst_width + 1) / 2,
                     dst_i420_opt + dst_i420_y_size + dst_i420_uv_size,
                       (dst_width + 1) / 2,
                     src_width, src_height, mode);
  }

  // Rotation should be exact.
  for (int i = 0; i < dst_i420_size; ++i) {
    EXPECT_EQ(dst_i420_c[i], dst_i420_opt[i]);
  }

  free_aligned_buffer_64(dst_i420_c);
  free_aligned_buffer_64(dst_i420_opt);
  free_aligned_buffer_64(src_nv12);
}

TEST_F(libyuvTest, NV12Rotate0) {
  NV12TestRotate(benchmark_width_, benchmark_height_,
                 benchmark_width_, benchmark_height_,
                 kRotate0, benchmark_iterations_);
}

TEST_F(libyuvTest, NV12Rotate90) {
  NV12TestRotate(benchmark_width_, benchmark_height_,
                 benchmark_height_, benchmark_width_,
                 kRotate90, benchmark_iterations_);
}

TEST_F(libyuvTest, NV12Rotate180) {
  NV12TestRotate(benchmark_width_, benchmark_height_,
                 benchmark_width_, benchmark_height_,
                 kRotate180, benchmark_iterations_);
}

TEST_F(libyuvTest, NV12Rotate270) {
  NV12TestRotate(benchmark_width_, benchmark_height_,
                 benchmark_height_, benchmark_width_,
                 kRotate270, benchmark_iterations_);
}

TEST_F(libyuvTest, NV12Rotate0_Odd) {
  NV12TestRotate(benchmark_width_ - 3, benchmark_height_ - 1,
                 benchmark_width_ - 3, benchmark_height_ - 1,
                 kRotate0, benchmark_iterations_);
}

TEST_F(libyuvTest, NV12Rotate90_Odd) {
  NV12TestRotate(benchmark_width_ - 3, benchmark_height_ - 1,
                 benchmark_height_ - 1, benchmark_width_ - 3,
                 kRotate90, benchmark_iterations_);
}

TEST_F(libyuvTest, NV12Rotate180_Odd) {
  NV12TestRotate(benchmark_width_ - 3, benchmark_height_ - 1,
                 benchmark_width_ - 3, benchmark_height_ - 1,
                 kRotate180, benchmark_iterations_);
}

TEST_F(libyuvTest, NV12Rotate270_Odd) {
  NV12TestRotate(benchmark_width_ - 3, benchmark_height_ - 1,
                 benchmark_height_ - 1, benchmark_width_ - 3,
                 kRotate270, benchmark_iterations_);
}

}  // namespace libyuv
