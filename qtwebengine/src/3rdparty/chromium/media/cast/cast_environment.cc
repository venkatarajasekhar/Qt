// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cast/cast_environment.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"

using base::SingleThreadTaskRunner;

namespace {

void DeleteLoggingOnMainThread(scoped_ptr<media::cast::LoggingImpl> logging) {
  logging.reset();
}

}  // namespace

namespace media {
namespace cast {

CastEnvironment::CastEnvironment(
    scoped_ptr<base::TickClock> clock,
    scoped_refptr<SingleThreadTaskRunner> main_thread_proxy,
    scoped_refptr<SingleThreadTaskRunner> audio_thread_proxy,
    scoped_refptr<SingleThreadTaskRunner> video_thread_proxy)
    : main_thread_proxy_(main_thread_proxy),
      audio_thread_proxy_(audio_thread_proxy),
      video_thread_proxy_(video_thread_proxy),
      clock_(clock.Pass()),
      logging_(new LoggingImpl) {}

CastEnvironment::~CastEnvironment() {
  // Logging must be deleted on the main thread.
  if (main_thread_proxy_ && !main_thread_proxy_->RunsTasksOnCurrentThread()) {
    main_thread_proxy_->PostTask(
        FROM_HERE,
        base::Bind(&DeleteLoggingOnMainThread, base::Passed(&logging_)));
  }
}

bool CastEnvironment::PostTask(ThreadId identifier,
                               const tracked_objects::Location& from_here,
                               const base::Closure& task) {
  return GetTaskRunner(identifier)->PostTask(from_here, task);
}

bool CastEnvironment::PostDelayedTask(
    ThreadId identifier,
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    base::TimeDelta delay) {
  return GetTaskRunner(identifier)->PostDelayedTask(from_here, task, delay);
}

scoped_refptr<SingleThreadTaskRunner> CastEnvironment::GetTaskRunner(
    ThreadId identifier) const {
  switch (identifier) {
    case CastEnvironment::MAIN:
      return main_thread_proxy_;
    case CastEnvironment::AUDIO:
      return audio_thread_proxy_;
    case CastEnvironment::VIDEO:
      return video_thread_proxy_;
    default:
      NOTREACHED() << "Invalid Thread identifier";
      return NULL;
  }
}

bool CastEnvironment::CurrentlyOn(ThreadId identifier) {
  switch (identifier) {
    case CastEnvironment::MAIN:
      return main_thread_proxy_ &&
             main_thread_proxy_->RunsTasksOnCurrentThread();
    case CastEnvironment::AUDIO:
      return audio_thread_proxy_ &&
             audio_thread_proxy_->RunsTasksOnCurrentThread();
    case CastEnvironment::VIDEO:
      return video_thread_proxy_ &&
             video_thread_proxy_->RunsTasksOnCurrentThread();
    default:
      NOTREACHED() << "Invalid thread identifier";
      return false;
  }
}

}  // namespace cast
}  // namespace media
