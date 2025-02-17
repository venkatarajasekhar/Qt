// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/statistics_recorder.h"
#include "base/test/test_timeouts.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_restrictions.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"
#include "dbus/test_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace dbus {

// The test for sender verification in ObjectProxy.
class SignalSenderVerificationTest : public testing::Test {
 public:
  SignalSenderVerificationTest()
      : on_name_owner_changed_called_(false),
        on_ownership_called_(false) {
  }

  virtual void SetUp() {
    base::StatisticsRecorder::Initialize();

    // Make the main thread not to allow IO.
    base::ThreadRestrictions::SetIOAllowed(false);

    // Start the D-Bus thread.
    dbus_thread_.reset(new base::Thread("D-Bus Thread"));
    base::Thread::Options thread_options;
    thread_options.message_loop_type = base::MessageLoop::TYPE_IO;
    ASSERT_TRUE(dbus_thread_->StartWithOptions(thread_options));

    // Create the client, using the D-Bus thread.
    Bus::Options bus_options;
    bus_options.bus_type = Bus::SESSION;
    bus_options.connection_type = Bus::PRIVATE;
    bus_options.dbus_task_runner = dbus_thread_->message_loop_proxy();
    bus_ = new Bus(bus_options);
    object_proxy_ = bus_->GetObjectProxy(
        "org.chromium.TestService",
        ObjectPath("/org/chromium/TestObject"));
    ASSERT_TRUE(bus_->HasDBusThread());

    object_proxy_->SetNameOwnerChangedCallback(
        base::Bind(&SignalSenderVerificationTest::OnNameOwnerChanged,
                   base::Unretained(this),
                   &on_name_owner_changed_called_));

    // Connect to the "Test" signal of "org.chromium.TestInterface" from
    // the remote object.
    object_proxy_->ConnectToSignal(
        "org.chromium.TestInterface",
        "Test",
        base::Bind(&SignalSenderVerificationTest::OnTestSignal,
                   base::Unretained(this)),
        base::Bind(&SignalSenderVerificationTest::OnConnected,
                   base::Unretained(this)));
    // Wait until the object proxy is connected to the signal.
    message_loop_.Run();

    // Start the test service, using the D-Bus thread.
    TestService::Options options;
    options.dbus_task_runner = dbus_thread_->message_loop_proxy();
    test_service_.reset(new TestService(options));
    ASSERT_TRUE(test_service_->StartService());
    ASSERT_TRUE(test_service_->WaitUntilServiceIsStarted());
    ASSERT_TRUE(test_service_->HasDBusThread());
    ASSERT_TRUE(test_service_->has_ownership());

    // Same setup for the second TestService. This service should not have the
    // ownership of the name at this point.
    test_service2_.reset(new TestService(options));
    ASSERT_TRUE(test_service2_->StartService());
    ASSERT_TRUE(test_service2_->WaitUntilServiceIsStarted());
    ASSERT_TRUE(test_service2_->HasDBusThread());
    ASSERT_FALSE(test_service2_->has_ownership());

    // The name should be owned and known at this point.
    if (!on_name_owner_changed_called_)
      message_loop_.Run();
    ASSERT_FALSE(latest_name_owner_.empty());
  }

  virtual void TearDown() {
    bus_->ShutdownOnDBusThreadAndBlock();

    // Shut down the service.
    test_service_->ShutdownAndBlock();
    test_service2_->ShutdownAndBlock();

    // Reset to the default.
    base::ThreadRestrictions::SetIOAllowed(true);

    // Stopping a thread is considered an IO operation, so do this after
    // allowing IO.
    test_service_->Stop();
    test_service2_->Stop();
  }

  void OnOwnership(bool expected, bool success) {
    ASSERT_EQ(expected, success);
    // PostTask to quit the MessageLoop as this is called from D-Bus thread.
    message_loop_.PostTask(
        FROM_HERE,
        base::Bind(&SignalSenderVerificationTest::OnOwnershipInternal,
                   base::Unretained(this)));
  }

  void OnOwnershipInternal() {
    on_ownership_called_ = true;
    message_loop_.Quit();
  }

  void OnNameOwnerChanged(bool* called_flag,
                          const std::string& old_owner,
                          const std::string& new_owner) {
    latest_name_owner_ = new_owner;
    *called_flag = true;
    message_loop_.Quit();
  }

  // Called when the "Test" signal is received, in the main thread.
  // Copy the string payload to |test_signal_string_|.
  void OnTestSignal(Signal* signal) {
    MessageReader reader(signal);
    ASSERT_TRUE(reader.PopString(&test_signal_string_));
    message_loop_.Quit();
  }

  // Called when connected to the signal.
  void OnConnected(const std::string& interface_name,
                   const std::string& signal_name,
                   bool success) {
    ASSERT_TRUE(success);
    message_loop_.Quit();
  }

 protected:
  // Wait for the hey signal to be received.
  void WaitForTestSignal() {
    // OnTestSignal() will quit the message loop.
    message_loop_.Run();
  }

  // Stopping a thread is considered an IO operation, so we need to fiddle with
  // thread restrictions before and after calling Stop() on a TestService.
  void SafeServiceStop(TestService* test_service) {
    base::ThreadRestrictions::SetIOAllowed(true);
    test_service->Stop();
    base::ThreadRestrictions::SetIOAllowed(false);
  }

  base::MessageLoop message_loop_;
  scoped_ptr<base::Thread> dbus_thread_;
  scoped_refptr<Bus> bus_;
  ObjectProxy* object_proxy_;
  scoped_ptr<TestService> test_service_;
  scoped_ptr<TestService> test_service2_;
  // Text message from "Test" signal.
  std::string test_signal_string_;

  // The known latest name owner of TestService. Updated in OnNameOwnerChanged.
  std::string latest_name_owner_;

  // Boolean flags to record callback calls.
  bool on_name_owner_changed_called_;
  bool on_ownership_called_;
};

TEST_F(SignalSenderVerificationTest, TestSignalAccepted) {
  const char kMessage[] = "hello, world";
  // Send the test signal from the exported object.
  test_service_->SendTestSignal(kMessage);
  // Receive the signal with the object proxy. The signal is handled in
  // SignalSenderVerificationTest::OnTestSignal() in the main thread.
  WaitForTestSignal();
  ASSERT_EQ(kMessage, test_signal_string_);
}

TEST_F(SignalSenderVerificationTest, TestSignalRejected) {
  // To make sure the histogram instance is created.
  UMA_HISTOGRAM_COUNTS("DBus.RejectedSignalCount", 0);
  base::HistogramBase* reject_signal_histogram =
        base::StatisticsRecorder::FindHistogram("DBus.RejectedSignalCount");
  scoped_ptr<base::HistogramSamples> samples1(
      reject_signal_histogram->SnapshotSamples());

  const char kNewMessage[] = "hello, new world";
  test_service2_->SendTestSignal(kNewMessage);

  // This test tests that our callback is NOT called by the ObjectProxy.
  // Sleep to have message delivered to the client via the D-Bus service.
  base::PlatformThread::Sleep(TestTimeouts::action_timeout());

  scoped_ptr<base::HistogramSamples> samples2(
      reject_signal_histogram->SnapshotSamples());

  ASSERT_EQ("", test_signal_string_);
  EXPECT_EQ(samples1->TotalCount() + 1, samples2->TotalCount());
}

TEST_F(SignalSenderVerificationTest, TestOwnerChanged) {
  const char kMessage[] = "hello, world";

  // Send the test signal from the exported object.
  test_service_->SendTestSignal(kMessage);
  // Receive the signal with the object proxy. The signal is handled in
  // SignalSenderVerificationTest::OnTestSignal() in the main thread.
  WaitForTestSignal();
  ASSERT_EQ(kMessage, test_signal_string_);

  // Release and acquire the name ownership.
  // latest_name_owner_ should be non empty as |test_service_| owns the name.
  ASSERT_FALSE(latest_name_owner_.empty());
  test_service_->ShutdownAndBlock();
  // OnNameOwnerChanged will PostTask to quit the message loop.
  message_loop_.Run();
  // latest_name_owner_ should be empty as the owner is gone.
  ASSERT_TRUE(latest_name_owner_.empty());

  // Reset the flag as NameOwnerChanged is already received in setup.
  on_name_owner_changed_called_ = false;
  on_ownership_called_ = false;
  test_service2_->RequestOwnership(
      base::Bind(&SignalSenderVerificationTest::OnOwnership,
                 base::Unretained(this), true));
  // Both of OnNameOwnerChanged() and OnOwnership() should quit the MessageLoop,
  // but there's no expected order of those 2 event.
  message_loop_.Run();
  if (!on_name_owner_changed_called_ || !on_ownership_called_)
    message_loop_.Run();
  ASSERT_TRUE(on_name_owner_changed_called_);
  ASSERT_TRUE(on_ownership_called_);

  // latest_name_owner_ becomes non empty as the new owner appears.
  ASSERT_FALSE(latest_name_owner_.empty());

  // Now the second service owns the name.
  const char kNewMessage[] = "hello, new world";

  test_service2_->SendTestSignal(kNewMessage);
  WaitForTestSignal();
  ASSERT_EQ(kNewMessage, test_signal_string_);
}

TEST_F(SignalSenderVerificationTest, TestOwnerStealing) {
  // Release and acquire the name ownership.
  // latest_name_owner_ should be non empty as |test_service_| owns the name.
  ASSERT_FALSE(latest_name_owner_.empty());
  test_service_->ShutdownAndBlock();
  // OnNameOwnerChanged will PostTask to quit the message loop.
  message_loop_.Run();
  // latest_name_owner_ should be empty as the owner is gone.
  ASSERT_TRUE(latest_name_owner_.empty());
  // Reset the flag as NameOwnerChanged is already received in setup.
  on_name_owner_changed_called_ = false;

  // Start a test service that allows theft, using the D-Bus thread.
  TestService::Options options;
  options.dbus_task_runner = dbus_thread_->message_loop_proxy();
  options.request_ownership_options = Bus::REQUIRE_PRIMARY_ALLOW_REPLACEMENT;
  TestService stealable_test_service(options);
  ASSERT_TRUE(stealable_test_service.StartService());
  ASSERT_TRUE(stealable_test_service.WaitUntilServiceIsStarted());
  ASSERT_TRUE(stealable_test_service.HasDBusThread());
  ASSERT_TRUE(stealable_test_service.has_ownership());

  // OnNameOwnerChanged will PostTask to quit the message loop.
  message_loop_.Run();

  // Send a signal to check that the service is correctly owned.
  const char kMessage[] = "hello, world";

  // Send the test signal from the exported object.
  stealable_test_service.SendTestSignal(kMessage);
  // Receive the signal with the object proxy. The signal is handled in
  // SignalSenderVerificationTest::OnTestSignal() in the main thread.
  WaitForTestSignal();
  ASSERT_EQ(kMessage, test_signal_string_);

  // Reset the flag as NameOwnerChanged was called above.
  on_name_owner_changed_called_ = false;
  test_service2_->RequestOwnership(
      base::Bind(&SignalSenderVerificationTest::OnOwnership,
                 base::Unretained(this), true));
  // Both of OnNameOwnerChanged() and OnOwnership() should quit the MessageLoop,
  // but there's no expected order of those 2 event.
  message_loop_.Run();
  if (!on_name_owner_changed_called_ || !on_ownership_called_)
    message_loop_.Run();
  ASSERT_TRUE(on_name_owner_changed_called_);
  ASSERT_TRUE(on_ownership_called_);

  // Now the second service owns the name.
  const char kNewMessage[] = "hello, new world";

  test_service2_->SendTestSignal(kNewMessage);
  WaitForTestSignal();
  ASSERT_EQ(kNewMessage, test_signal_string_);

  SafeServiceStop(&stealable_test_service);
}

// Fails on Linux ChromiumOS Tests
TEST_F(SignalSenderVerificationTest, DISABLED_TestMultipleObjects) {
  const char kMessage[] = "hello, world";

  ObjectProxy* object_proxy2 = bus_->GetObjectProxy(
      "org.chromium.TestService",
      ObjectPath("/org/chromium/DifferentObject"));

  bool second_name_owner_changed_called = false;
  object_proxy2->SetNameOwnerChangedCallback(
      base::Bind(&SignalSenderVerificationTest::OnNameOwnerChanged,
                 base::Unretained(this),
                 &second_name_owner_changed_called));

  // Connect to a signal on the additional remote object to trigger the
  // name owner matching.
  object_proxy2->ConnectToSignal(
      "org.chromium.DifferentTestInterface",
      "Test",
      base::Bind(&SignalSenderVerificationTest::OnTestSignal,
                 base::Unretained(this)),
      base::Bind(&SignalSenderVerificationTest::OnConnected,
                 base::Unretained(this)));
  // Wait until the object proxy is connected to the signal.
  message_loop_.Run();

  // Send the test signal from the exported object.
  test_service_->SendTestSignal(kMessage);
  // Receive the signal with the object proxy. The signal is handled in
  // SignalSenderVerificationTest::OnTestSignal() in the main thread.
  WaitForTestSignal();
  ASSERT_EQ(kMessage, test_signal_string_);

  // Release and acquire the name ownership.
  // latest_name_owner_ should be non empty as |test_service_| owns the name.
  ASSERT_FALSE(latest_name_owner_.empty());
  test_service_->ShutdownAndBlock();
  // OnNameOwnerChanged will PostTask to quit the message loop.
  message_loop_.Run();
  // latest_name_owner_ should be empty as the owner is gone.
  ASSERT_TRUE(latest_name_owner_.empty());

  // Reset the flag as NameOwnerChanged is already received in setup.
  on_name_owner_changed_called_ = false;
  second_name_owner_changed_called = false;
  test_service2_->RequestOwnership(
      base::Bind(&SignalSenderVerificationTest::OnOwnership,
                 base::Unretained(this), true));
  // Both of OnNameOwnerChanged() and OnOwnership() should quit the MessageLoop,
  // but there's no expected order of those 2 event.
  while (!on_name_owner_changed_called_ || !second_name_owner_changed_called ||
         !on_ownership_called_)
    message_loop_.Run();
  ASSERT_TRUE(on_name_owner_changed_called_);
  ASSERT_TRUE(second_name_owner_changed_called);
  ASSERT_TRUE(on_ownership_called_);

  // latest_name_owner_ becomes non empty as the new owner appears.
  ASSERT_FALSE(latest_name_owner_.empty());

  // Now the second service owns the name.
  const char kNewMessage[] = "hello, new world";

  test_service2_->SendTestSignal(kNewMessage);
  WaitForTestSignal();
  ASSERT_EQ(kNewMessage, test_signal_string_);
}

}  // namespace dbus
