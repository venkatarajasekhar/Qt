/*
 * libjingle
 * Copyright 2004--2011, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "talk/base/gunit.h"
#include "talk/base/signalthread.h"
#include "talk/base/thread.h"

using namespace talk_base;

class SignalThreadTest : public testing::Test, public sigslot::has_slots<> {
 public:
  class SlowSignalThread : public SignalThread {
   public:
    SlowSignalThread(SignalThreadTest* harness) : harness_(harness) {
    }

    virtual ~SlowSignalThread() {
      EXPECT_EQ(harness_->main_thread_, Thread::Current());
      ++harness_->thread_deleted_;
    }

    const SignalThreadTest* harness() { return harness_; }

   protected:
    virtual void OnWorkStart() {
      ASSERT_TRUE(harness_ != NULL);
      ++harness_->thread_started_;
      EXPECT_EQ(harness_->main_thread_, Thread::Current());
      EXPECT_FALSE(worker()->RunningForTest());  // not started yet
    }

    virtual void OnWorkStop() {
      ++harness_->thread_stopped_;
      EXPECT_EQ(harness_->main_thread_, Thread::Current());
      EXPECT_TRUE(worker()->RunningForTest());  // not stopped yet
    }

    virtual void OnWorkDone() {
      ++harness_->thread_done_;
      EXPECT_EQ(harness_->main_thread_, Thread::Current());
      EXPECT_TRUE(worker()->RunningForTest());  // not stopped yet
    }

    virtual void DoWork() {
      EXPECT_NE(harness_->main_thread_, Thread::Current());
      EXPECT_EQ(worker(), Thread::Current());
      Thread::Current()->socketserver()->Wait(250, false);
    }

   private:
    SignalThreadTest* harness_;
    DISALLOW_EVIL_CONSTRUCTORS(SlowSignalThread);
  };

  void OnWorkComplete(talk_base::SignalThread* thread) {
    SlowSignalThread* t = static_cast<SlowSignalThread*>(thread);
    EXPECT_EQ(t->harness(), this);
    EXPECT_EQ(main_thread_, Thread::Current());

    ++thread_completed_;
    if (!called_release_) {
      thread->Release();
    }
  }

  virtual void SetUp() {
    main_thread_ = Thread::Current();
    thread_ = new SlowSignalThread(this);
    thread_->SignalWorkDone.connect(this, &SignalThreadTest::OnWorkComplete);
    called_release_ = false;
    thread_started_ = 0;
    thread_done_ = 0;
    thread_completed_ = 0;
    thread_stopped_ = 0;
    thread_deleted_ = 0;
  }

  virtual void TearDown() {
  }

  Thread* main_thread_;
  SlowSignalThread* thread_;
  bool called_release_;

  int thread_started_;
  int thread_done_;
  int thread_completed_;
  int thread_stopped_;
  int thread_deleted_;
};

class OwnerThread : public Thread, public sigslot::has_slots<> {
 public:
  explicit OwnerThread(SignalThreadTest* harness)
      : harness_(harness),
        has_run_(false) {
  }

  virtual ~OwnerThread() {
    Stop();
  }

  virtual void Run() {
    SignalThreadTest::SlowSignalThread* signal_thread =
        new SignalThreadTest::SlowSignalThread(harness_);
    signal_thread->SignalWorkDone.connect(this, &OwnerThread::OnWorkDone);
    signal_thread->Start();
    Thread::Current()->socketserver()->Wait(100, false);
    signal_thread->Release();
    // Delete |signal_thread|.
    signal_thread->Destroy(true);
    has_run_ = true;
  }

  bool has_run() { return has_run_; }
  void OnWorkDone(SignalThread* signal_thread) {
    FAIL() << " This shouldn't get called.";
  }

 private:
  SignalThreadTest* harness_;
  bool has_run_;
  DISALLOW_EVIL_CONSTRUCTORS(OwnerThread);
};

// Test for when the main thread goes away while the
// signal thread is still working.  This may happen
// when shutting down the process.
TEST_F(SignalThreadTest, OwnerThreadGoesAway) {
  {
    scoped_ptr<OwnerThread> owner(new OwnerThread(this));
    main_thread_ = owner.get();
    owner->Start();
    while (!owner->has_run()) {
      Thread::Current()->socketserver()->Wait(10, false);
    }
  }
  // At this point the main thread has gone away.
  // Give the SignalThread a little time to do its callback,
  // which will crash if the signal thread doesn't handle
  // this situation well.
  Thread::Current()->socketserver()->Wait(500, false);
}

#define EXPECT_STATE(started, done, completed, stopped, deleted) \
  EXPECT_EQ(started, thread_started_); \
  EXPECT_EQ(done, thread_done_); \
  EXPECT_EQ(completed, thread_completed_); \
  EXPECT_EQ(stopped, thread_stopped_); \
  EXPECT_EQ(deleted, thread_deleted_);

TEST_F(SignalThreadTest, ThreadFinishes) {
  thread_->Start();
  EXPECT_STATE(1, 0, 0, 0, 0);
  Thread::SleepMs(500);
  EXPECT_STATE(1, 0, 0, 0, 0);
  Thread::Current()->ProcessMessages(0);
  EXPECT_STATE(1, 1, 1, 0, 1);
}

TEST_F(SignalThreadTest, ReleasedThreadFinishes) {
  thread_->Start();
  EXPECT_STATE(1, 0, 0, 0, 0);
  thread_->Release();
  called_release_ = true;
  EXPECT_STATE(1, 0, 0, 0, 0);
  Thread::SleepMs(500);
  EXPECT_STATE(1, 0, 0, 0, 0);
  Thread::Current()->ProcessMessages(0);
  EXPECT_STATE(1, 1, 1, 0, 1);
}

TEST_F(SignalThreadTest, DestroyedThreadCleansUp) {
  thread_->Start();
  EXPECT_STATE(1, 0, 0, 0, 0);
  thread_->Destroy(true);
  EXPECT_STATE(1, 0, 0, 1, 1);
  Thread::Current()->ProcessMessages(0);
  EXPECT_STATE(1, 0, 0, 1, 1);
}

TEST_F(SignalThreadTest, DeferredDestroyedThreadCleansUp) {
  thread_->Start();
  EXPECT_STATE(1, 0, 0, 0, 0);
  thread_->Destroy(false);
  EXPECT_STATE(1, 0, 0, 1, 0);
  Thread::SleepMs(500);
  EXPECT_STATE(1, 0, 0, 1, 0);
  Thread::Current()->ProcessMessages(0);
  EXPECT_STATE(1, 1, 0, 1, 1);
}
