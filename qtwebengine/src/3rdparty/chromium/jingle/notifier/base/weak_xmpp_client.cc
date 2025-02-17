// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/notifier/base/weak_xmpp_client.h"

#include "base/compiler_specific.h"

namespace notifier {

WeakXmppClient::WeakXmppClient(talk_base::TaskParent* parent)
    : buzz::XmppClient(parent),
      weak_ptr_factory_(this) {}

WeakXmppClient::~WeakXmppClient() {
  DCHECK(CalledOnValidThread());
  Invalidate();
}

void WeakXmppClient::Invalidate() {
  DCHECK(CalledOnValidThread());
  // We don't want XmppClient raising any signals once its invalidated.
  SignalStateChange.disconnect_all();
  SignalLogInput.disconnect_all();
  SignalLogOutput.disconnect_all();
  weak_ptr_factory_.InvalidateWeakPtrs();
}

base::WeakPtr<WeakXmppClient> WeakXmppClient::AsWeakPtr() {
  DCHECK(CalledOnValidThread());
  return weak_ptr_factory_.GetWeakPtr();
}

void WeakXmppClient::Stop() {
  DCHECK(CalledOnValidThread());
  // We don't want XmppClient used after it has been stopped.
  Invalidate();
  buzz::XmppClient::Stop();
}

}  // namespace notifier
