// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/test_tools/quic_flow_controller_peer.h"

#include <list>

#include "net/quic/quic_flow_controller.h"
#include "net/quic/quic_protocol.h"

namespace net {
namespace test {

// static
void QuicFlowControllerPeer::SetSendWindowOffset(
    QuicFlowController* flow_controller,
    uint64 offset) {
  flow_controller->send_window_offset_ = offset;
}

// static
void QuicFlowControllerPeer::SetReceiveWindowOffset(
    QuicFlowController* flow_controller,
    uint64 offset) {
  flow_controller->receive_window_offset_ = offset;
}

// static
void QuicFlowControllerPeer::SetMaxReceiveWindow(
    QuicFlowController* flow_controller, uint64 window_size) {
  flow_controller->max_receive_window_ = window_size;
}

// static
uint64 QuicFlowControllerPeer::SendWindowOffset(
    QuicFlowController* flow_controller) {
  return flow_controller->send_window_offset_;
}

// static
uint64 QuicFlowControllerPeer::SendWindowSize(
    QuicFlowController* flow_controller) {
  return flow_controller->SendWindowSize();
}

// static
uint64 QuicFlowControllerPeer::ReceiveWindowOffset(
    QuicFlowController* flow_controller) {
  return flow_controller->receive_window_offset_;
}

// static
uint64 QuicFlowControllerPeer::ReceiveWindowSize(
    QuicFlowController* flow_controller) {
  return flow_controller->receive_window_offset_ -
         flow_controller->highest_received_byte_offset_;
}

}  // namespace test
}  // namespace net
