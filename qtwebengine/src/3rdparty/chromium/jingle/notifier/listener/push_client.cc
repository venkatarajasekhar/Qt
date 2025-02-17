// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/notifier/listener/push_client.h"

#include <cstddef>

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "jingle/notifier/listener/non_blocking_push_client.h"
#include "jingle/notifier/listener/xmpp_push_client.h"

namespace notifier {

PushClient::~PushClient() {}

namespace {

scoped_ptr<PushClient> CreateXmppPushClient(
    const NotifierOptions& notifier_options) {
  return scoped_ptr<PushClient>(new XmppPushClient(notifier_options));
}

}  // namespace

scoped_ptr<PushClient> PushClient::CreateDefault(
    const NotifierOptions& notifier_options) {
  return scoped_ptr<PushClient>(new NonBlockingPushClient(
      notifier_options.request_context_getter->GetNetworkTaskRunner(),
      base::Bind(&CreateXmppPushClient, notifier_options)));
}

scoped_ptr<PushClient> PushClient::CreateDefaultOnIOThread(
    const NotifierOptions& notifier_options) {
  CHECK(notifier_options.request_context_getter->GetNetworkTaskRunner()->
        BelongsToCurrentThread());
  return CreateXmppPushClient(notifier_options);
}

}  // namespace notifier
