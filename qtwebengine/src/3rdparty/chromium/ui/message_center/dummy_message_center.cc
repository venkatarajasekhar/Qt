// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/message_center.h"

// This file contains dummy implementation of MessageCenter and used to compile
// and link with Android and iOS implementations of Chrome which do not have
// notification systems yet. This is to avoid spreading compile-time flags
// everywhere in the code.
#if !defined(OS_ANDROID) && !defined(OS_IOS)
#error This file should only be used in Android or iOS builds.
#endif

namespace message_center {

// static
void MessageCenter::Initialize() {
}

// static
MessageCenter* MessageCenter::Get() {
  return NULL;
}

// static
void MessageCenter::Shutdown() {
}

MessageCenter::MessageCenter() {
}

MessageCenter::~MessageCenter() {
}

}  // namespace message_center
