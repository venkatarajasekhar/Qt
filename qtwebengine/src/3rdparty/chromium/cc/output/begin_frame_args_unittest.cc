// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "cc/output/begin_frame_args.h"
#include "cc/test/begin_frame_args_test.h"
#include "testing/gtest/include/gtest/gtest-spi.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/frame_time.h"

namespace cc {
namespace {

TEST(BeginFrameArgsTest, Helpers) {
  // Quick create methods work
  BeginFrameArgs args0 = CreateBeginFrameArgsForTesting();
  EXPECT_TRUE(args0.IsValid()) << args0;

  BeginFrameArgs args1 = CreateBeginFrameArgsForTesting(0, 0, -1);
  EXPECT_FALSE(args1.IsValid()) << args1;

  BeginFrameArgs args2 = CreateBeginFrameArgsForTesting(1, 2, 3);
  EXPECT_TRUE(args2.IsValid()) << args2;
  EXPECT_EQ(1, args2.frame_time.ToInternalValue());
  EXPECT_EQ(2, args2.deadline.ToInternalValue());
  EXPECT_EQ(3, args2.interval.ToInternalValue());

  BeginFrameArgs args3 = CreateExpiredBeginFrameArgsForTesting();
  EXPECT_TRUE(args3.IsValid()) << args3;
  EXPECT_GT(gfx::FrameTime::Now(), args3.deadline);

  // operator==
  EXPECT_EQ(CreateBeginFrameArgsForTesting(4, 5, 6),
            CreateBeginFrameArgsForTesting(4, 5, 6));

  EXPECT_NONFATAL_FAILURE(EXPECT_EQ(CreateBeginFrameArgsForTesting(4, 5, 6),
                                    CreateBeginFrameArgsForTesting(7, 8, 9)),
                          "");

  // operator<<
  std::stringstream out1;
  out1 << args1;
  EXPECT_EQ("BeginFrameArgs(0, 0, -1us)", out1.str());
  std::stringstream out2;
  out2 << args2;
  EXPECT_EQ("BeginFrameArgs(1, 2, 3us)", out2.str());

  // PrintTo
  EXPECT_EQ(std::string("BeginFrameArgs(0, 0, -1us)"),
            ::testing::PrintToString(args1));
  EXPECT_EQ(std::string("BeginFrameArgs(1, 2, 3us)"),
            ::testing::PrintToString(args2));
}

TEST(BeginFrameArgsTest, Create) {
  // BeginFrames are not valid by default
  BeginFrameArgs args1;
  EXPECT_FALSE(args1.IsValid()) << args1;

  BeginFrameArgs args2 =
      BeginFrameArgs::Create(base::TimeTicks::FromInternalValue(1),
                             base::TimeTicks::FromInternalValue(2),
                             base::TimeDelta::FromInternalValue(3));
  EXPECT_TRUE(args2.IsValid()) << args2;
  EXPECT_EQ(1, args2.frame_time.ToInternalValue()) << args2;
  EXPECT_EQ(2, args2.deadline.ToInternalValue()) << args2;
  EXPECT_EQ(3, args2.interval.ToInternalValue()) << args2;

  base::TimeTicks now = base::TimeTicks::FromInternalValue(1);
  EXPECT_EQ(CreateBeginFrameArgsForTesting(1, 0, 16666),
            BeginFrameArgs::CreateForSynchronousCompositor(now));
}

}  // namespace
}  // namespace cc
