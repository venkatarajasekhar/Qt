// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "cc/base/unique_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

class UniqueNotifierTest : public testing::Test {
 public:
  UniqueNotifierTest() : notification_count_(0) {}

  virtual void SetUp() OVERRIDE { ResetNotificationCount(); }

  void Notify() { ++notification_count_; }

  int NotificationCount() const { return notification_count_; }

  void ResetNotificationCount() { notification_count_ = 0; }

 protected:
  int notification_count_;
};

TEST_F(UniqueNotifierTest, Schedule) {
  {
    UniqueNotifier notifier(
        base::MessageLoopProxy::current(),
        base::Bind(&UniqueNotifierTest::Notify, base::Unretained(this)));

    EXPECT_EQ(0, NotificationCount());

    // Basic schedule should result in a run.
    notifier.Schedule();

    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(1, NotificationCount());

    // Multiple schedules should only result in one run.
    for (int i = 0; i < 5; ++i)
      notifier.Schedule();

    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(2, NotificationCount());

    notifier.Schedule();
  }

  // Notifier went out of scope, so we don't expect to get a notification.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, NotificationCount());
}

}  // namespace
}  // namespace cc
