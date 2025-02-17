// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/test/test_pending_task.h"
#include "base/test/test_simple_task_runner.h"
#include "device/bluetooth/bluetooth_init_win.h"
#include "device/bluetooth/bluetooth_task_manager_win.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class BluetoothTaskObserver : public device::BluetoothTaskManagerWin::Observer {
 public:
  BluetoothTaskObserver()
      : num_adapter_state_changed_(0),
        num_discovery_started_(0),
        num_discovery_stopped_(0) {
  }

  virtual ~BluetoothTaskObserver() {
  }

  virtual void AdapterStateChanged(
    const device::BluetoothTaskManagerWin::AdapterState& state) OVERRIDE {
    num_adapter_state_changed_++;
  }

  virtual void DiscoveryStarted(bool success) OVERRIDE {
    num_discovery_started_++;
  }

  virtual void DiscoveryStopped() OVERRIDE {
    num_discovery_stopped_++;
  }

  int num_adapter_state_changed() const {
    return num_adapter_state_changed_;
  }

  int num_discovery_started() const {
    return num_discovery_started_;
  }

  int num_discovery_stopped() const {
    return num_discovery_stopped_;
  }

 private:
   int num_adapter_state_changed_;
   int num_discovery_started_;
   int num_discovery_stopped_;
};

}  // namespace

namespace device {

class BluetoothTaskManagerWinTest : public testing::Test {
 public:
  BluetoothTaskManagerWinTest()
      : ui_task_runner_(new base::TestSimpleTaskRunner()),
        bluetooth_task_runner_(new base::TestSimpleTaskRunner()),
        task_manager_(new BluetoothTaskManagerWin(ui_task_runner_)),
        has_bluetooth_stack_(device::bluetooth_init_win::HasBluetoothStack()) {
    task_manager_->InitializeWithBluetoothTaskRunner(bluetooth_task_runner_);
  }

  virtual void SetUp() {
    task_manager_->AddObserver(&observer_);
  }

  virtual void TearDown() {
    task_manager_->RemoveObserver(&observer_);
  }

  int GetPollingIntervalMs() const {
    return BluetoothTaskManagerWin::kPollIntervalMs;
  }

 protected:
  scoped_refptr<base::TestSimpleTaskRunner> ui_task_runner_;
  scoped_refptr<base::TestSimpleTaskRunner> bluetooth_task_runner_;
  scoped_refptr<BluetoothTaskManagerWin> task_manager_;
  BluetoothTaskObserver observer_;
  const bool has_bluetooth_stack_;
};

TEST_F(BluetoothTaskManagerWinTest, StartPolling) {
  EXPECT_EQ(1, bluetooth_task_runner_->GetPendingTasks().size());
}

TEST_F(BluetoothTaskManagerWinTest, PollAdapterIfBluetoothStackIsAvailable) {
  bluetooth_task_runner_->RunPendingTasks();
  int num_expected_pending_tasks = has_bluetooth_stack_ ? 1 : 0;
  EXPECT_EQ(num_expected_pending_tasks,
            bluetooth_task_runner_->GetPendingTasks().size());
}

TEST_F(BluetoothTaskManagerWinTest, Polling) {
  if (!has_bluetooth_stack_)
    return;

  int num_polls = 5;

  for (int i = 0; i < num_polls; i++) {
    bluetooth_task_runner_->RunPendingTasks();
  }

  ui_task_runner_->RunPendingTasks();
  EXPECT_EQ(num_polls, observer_.num_adapter_state_changed());
}

TEST_F(BluetoothTaskManagerWinTest, SetPowered) {
  if (!has_bluetooth_stack_)
    return;

  bluetooth_task_runner_->ClearPendingTasks();
  base::Closure closure;
  task_manager_->PostSetPoweredBluetoothTask(true, closure, closure);

  EXPECT_EQ(1, bluetooth_task_runner_->GetPendingTasks().size());
  bluetooth_task_runner_->RunPendingTasks();
  EXPECT_TRUE(ui_task_runner_->GetPendingTasks().size() >= 1);
}

TEST_F(BluetoothTaskManagerWinTest, Discovery) {
  if (!has_bluetooth_stack_)
    return;

  bluetooth_task_runner_->RunPendingTasks();
  bluetooth_task_runner_->ClearPendingTasks();
  task_manager_->PostStartDiscoveryTask();
  bluetooth_task_runner_->RunPendingTasks();
  ui_task_runner_->RunPendingTasks();
  EXPECT_EQ(1, observer_.num_discovery_started());
  task_manager_->PostStopDiscoveryTask();
  bluetooth_task_runner_->RunPendingTasks();
  ui_task_runner_->RunPendingTasks();
  EXPECT_EQ(1, observer_.num_discovery_stopped());
}

}  // namespace device
