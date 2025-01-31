// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/system/message_in_transit_queue.h"

#include "base/logging.h"
#include "base/stl_util.h"

namespace mojo {
namespace system {

MessageInTransitQueue::MessageInTransitQueue() {
}

MessageInTransitQueue::MessageInTransitQueue(PassContents,
                                             MessageInTransitQueue* other) {
  queue_.swap(other->queue_);
}

MessageInTransitQueue::~MessageInTransitQueue() {
  if (!IsEmpty()) {
    LOG(WARNING) << "Destroying nonempty message queue";
    Clear();
  }
}

void MessageInTransitQueue::Clear() {
  STLDeleteElements(&queue_);
}

}  // namespace system
}  // namespace mojo
