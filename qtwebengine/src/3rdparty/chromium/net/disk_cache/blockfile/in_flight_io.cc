// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/blockfile/in_flight_io.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/threading/thread_restrictions.h"

namespace disk_cache {

BackgroundIO::BackgroundIO(InFlightIO* controller)
    : result_(-1), io_completed_(true, false), controller_(controller) {
}

// Runs on the primary thread.
void BackgroundIO::OnIOSignalled() {
  if (controller_)
    controller_->InvokeCallback(this, false);
}

void BackgroundIO::Cancel() {
  // controller_ may be in use from the background thread at this time.
  base::AutoLock lock(controller_lock_);
  DCHECK(controller_);
  controller_ = NULL;
}

BackgroundIO::~BackgroundIO() {
}

// ---------------------------------------------------------------------------

InFlightIO::InFlightIO()
    : callback_thread_(base::MessageLoopProxy::current()),
      running_(false), single_thread_(false) {
}

InFlightIO::~InFlightIO() {
}

// Runs on the background thread.
void BackgroundIO::NotifyController() {
  base::AutoLock lock(controller_lock_);
  if (controller_)
    controller_->OnIOComplete(this);
}

void InFlightIO::WaitForPendingIO() {
  while (!io_list_.empty()) {
    // Block the current thread until all pending IO completes.
    IOList::iterator it = io_list_.begin();
    InvokeCallback(it->get(), true);
  }
}

void InFlightIO::DropPendingIO() {
  while (!io_list_.empty()) {
    IOList::iterator it = io_list_.begin();
    BackgroundIO* operation = it->get();
    operation->Cancel();
    DCHECK(io_list_.find(operation) != io_list_.end());
    io_list_.erase(make_scoped_refptr(operation));
  }
}

// Runs on a background thread.
void InFlightIO::OnIOComplete(BackgroundIO* operation) {
#ifndef NDEBUG
  if (callback_thread_->BelongsToCurrentThread()) {
    DCHECK(single_thread_ || !running_);
    single_thread_ = true;
  }
#endif

  callback_thread_->PostTask(FROM_HERE,
                             base::Bind(&BackgroundIO::OnIOSignalled,
                                        operation));
  operation->io_completed()->Signal();
}

// Runs on the primary thread.
void InFlightIO::InvokeCallback(BackgroundIO* operation, bool cancel_task) {
  {
    // http://crbug.com/74623
    base::ThreadRestrictions::ScopedAllowWait allow_wait;
    operation->io_completed()->Wait();
  }
  running_ = true;

  if (cancel_task)
    operation->Cancel();

  // Make sure that we remove the operation from the list before invoking the
  // callback (so that a subsequent cancel does not invoke the callback again).
  DCHECK(io_list_.find(operation) != io_list_.end());
  DCHECK(!operation->HasOneRef());
  io_list_.erase(make_scoped_refptr(operation));
  OnOperationComplete(operation, cancel_task);
}

// Runs on the primary thread.
void InFlightIO::OnOperationPosted(BackgroundIO* operation) {
  DCHECK(callback_thread_->BelongsToCurrentThread());
  io_list_.insert(make_scoped_refptr(operation));
}

}  // namespace disk_cache
