// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/lib/router.h"

namespace mojo {
namespace internal {

// ----------------------------------------------------------------------------

class ResponderThunk : public MessageReceiver {
 public:
  explicit ResponderThunk(const SharedData<Router*>& router)
      : router_(router) {
  }
  virtual ~ResponderThunk() {
  }

  // MessageReceiver implementation:
  virtual bool Accept(Message* message) MOJO_OVERRIDE {
    assert(message->has_flag(kMessageIsResponse));

    bool result = false;

    Router* router = router_.value();
    if (router)
      result = router->Accept(message);

    return result;
  }

 private:
  SharedData<Router*> router_;
};

// ----------------------------------------------------------------------------

Router::HandleIncomingMessageThunk::HandleIncomingMessageThunk(Router* router)
    : router_(router) {
}

Router::HandleIncomingMessageThunk::~HandleIncomingMessageThunk() {
}

bool Router::HandleIncomingMessageThunk::Accept(Message* message) {
  return router_->HandleIncomingMessage(message);
}

// ----------------------------------------------------------------------------

Router::Router(ScopedMessagePipeHandle message_pipe,
               FilterChain filters,
               const MojoAsyncWaiter* waiter)
    : thunk_(this),
      filters_(filters.Pass()),
      connector_(message_pipe.Pass(), waiter),
      weak_self_(this),
      incoming_receiver_(NULL),
      next_request_id_(0),
      testing_mode_(false) {
  filters_.SetSink(&thunk_);
  connector_.set_incoming_receiver(filters_.GetHead());
}

Router::~Router() {
  weak_self_.set_value(NULL);

  for (ResponderMap::const_iterator i = responders_.begin();
       i != responders_.end(); ++i) {
    delete i->second;
  }
}

bool Router::Accept(Message* message) {
  assert(!message->has_flag(kMessageExpectsResponse));
  return connector_.Accept(message);
}

bool Router::AcceptWithResponder(Message* message,
                                 MessageReceiver* responder) {
  assert(message->has_flag(kMessageExpectsResponse));

  // Reserve 0 in case we want it to convey special meaning in the future.
  uint64_t request_id = next_request_id_++;
  if (request_id == 0)
    request_id = next_request_id_++;

  message->set_request_id(request_id);
  if (!connector_.Accept(message))
    return false;

  // We assume ownership of |responder|.
  responders_[request_id] = responder;
  return true;
}

void Router::EnableTestingMode() {
  testing_mode_ = true;
  connector_.set_enforce_errors_from_incoming_receiver(false);
}

bool Router::HandleIncomingMessage(Message* message) {
  if (message->has_flag(kMessageExpectsResponse)) {
    if (incoming_receiver_) {
      MessageReceiver* responder = new ResponderThunk(weak_self_);
      bool ok = incoming_receiver_->AcceptWithResponder(message, responder);
      if (!ok)
        delete responder;
      return ok;
    }

    // If we receive a request expecting a response when the client is not
    // listening, then we have no choice but to tear down the pipe.
    connector_.CloseMessagePipe();
  } else if (message->has_flag(kMessageIsResponse)) {
    uint64_t request_id = message->request_id();
    ResponderMap::iterator it = responders_.find(request_id);
    if (it == responders_.end()) {
      assert(testing_mode_);
      return false;
    }
    MessageReceiver* responder = it->second;
    responders_.erase(it);
    bool ok = responder->Accept(message);
    delete responder;
    return ok;
  } else {
    if (incoming_receiver_)
      return incoming_receiver_->Accept(message);
    // OK to drop message on the floor.
  }

  return false;
}

// ----------------------------------------------------------------------------

}  // namespace internal
}  // namespace mojo
