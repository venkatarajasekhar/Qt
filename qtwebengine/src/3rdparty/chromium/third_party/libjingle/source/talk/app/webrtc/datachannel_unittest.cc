/*
 * libjingle
 * Copyright 2013, Google Inc.
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

#include "talk/app/webrtc/datachannel.h"
#include "talk/app/webrtc/sctputils.h"
#include "talk/app/webrtc/test/fakedatachannelprovider.h"
#include "talk/base/gunit.h"

using webrtc::DataChannel;

class FakeDataChannelObserver : public webrtc::DataChannelObserver {
 public:
  FakeDataChannelObserver()
      : messages_received_(0), on_state_change_count_(0) {}

  void OnStateChange() {
    ++on_state_change_count_;
  }

  void OnMessage(const webrtc::DataBuffer& buffer) {
    ++messages_received_;
  }

  size_t messages_received() const {
    return messages_received_;
  }

  void ResetOnStateChangeCount() {
    on_state_change_count_ = 0;
  }

  size_t on_state_change_count() const {
    return on_state_change_count_;
  }

 private:
  size_t messages_received_;
  size_t on_state_change_count_;
};

class SctpDataChannelTest : public testing::Test {
 protected:
  SctpDataChannelTest()
      : webrtc_data_channel_(
          DataChannel::Create(
              &provider_, cricket::DCT_SCTP, "test", init_)) {
  }

  void SetChannelReady() {
    provider_.set_transport_available(true);
    webrtc_data_channel_->OnTransportChannelCreated();
    if (webrtc_data_channel_->id() < 0) {
      webrtc_data_channel_->SetSctpSid(0);
    }
    provider_.set_ready_to_send(true);
  }

  void AddObserver() {
    observer_.reset(new FakeDataChannelObserver());
    webrtc_data_channel_->RegisterObserver(observer_.get());
  }

  webrtc::InternalDataChannelInit init_;
  FakeDataChannelProvider provider_;
  talk_base::scoped_ptr<FakeDataChannelObserver> observer_;
  talk_base::scoped_refptr<DataChannel> webrtc_data_channel_;
};

// Verifies that the data channel is connected to the transport after creation.
TEST_F(SctpDataChannelTest, ConnectedToTransportOnCreated) {
  provider_.set_transport_available(true);
  talk_base::scoped_refptr<DataChannel> dc = DataChannel::Create(
      &provider_, cricket::DCT_SCTP, "test1", init_);

  EXPECT_TRUE(provider_.IsConnected(dc.get()));
  // The sid is not set yet, so it should not have added the streams.
  EXPECT_FALSE(provider_.IsSendStreamAdded(dc->id()));
  EXPECT_FALSE(provider_.IsRecvStreamAdded(dc->id()));

  dc->SetSctpSid(0);
  EXPECT_TRUE(provider_.IsSendStreamAdded(dc->id()));
  EXPECT_TRUE(provider_.IsRecvStreamAdded(dc->id()));
}

// Verifies that the data channel is connected to the transport if the transport
// is not available initially and becomes available later.
TEST_F(SctpDataChannelTest, ConnectedAfterTransportBecomesAvailable) {
  EXPECT_FALSE(provider_.IsConnected(webrtc_data_channel_.get()));

  provider_.set_transport_available(true);
  webrtc_data_channel_->OnTransportChannelCreated();
  EXPECT_TRUE(provider_.IsConnected(webrtc_data_channel_.get()));
}

// Tests the state of the data channel.
TEST_F(SctpDataChannelTest, StateTransition) {
  EXPECT_EQ(webrtc::DataChannelInterface::kConnecting,
            webrtc_data_channel_->state());
  SetChannelReady();

  EXPECT_EQ(webrtc::DataChannelInterface::kOpen, webrtc_data_channel_->state());
  webrtc_data_channel_->Close();
  EXPECT_EQ(webrtc::DataChannelInterface::kClosed,
            webrtc_data_channel_->state());
  // Verifies that it's disconnected from the transport.
  EXPECT_FALSE(provider_.IsConnected(webrtc_data_channel_.get()));
}

// Tests that DataChannel::buffered_amount() is correct after the channel is
// blocked.
TEST_F(SctpDataChannelTest, BufferedAmountWhenBlocked) {
  SetChannelReady();
  webrtc::DataBuffer buffer("abcd");
  EXPECT_TRUE(webrtc_data_channel_->Send(buffer));

  EXPECT_EQ(0U, webrtc_data_channel_->buffered_amount());

  provider_.set_send_blocked(true);

  const int number_of_packets = 3;
  for (int i = 0; i < number_of_packets; ++i) {
    EXPECT_TRUE(webrtc_data_channel_->Send(buffer));
  }
  EXPECT_EQ(buffer.data.length() * number_of_packets,
            webrtc_data_channel_->buffered_amount());
}

// Tests that the queued data are sent when the channel transitions from blocked
// to unblocked.
TEST_F(SctpDataChannelTest, QueuedDataSentWhenUnblocked) {
  SetChannelReady();
  webrtc::DataBuffer buffer("abcd");
  provider_.set_send_blocked(true);
  EXPECT_TRUE(webrtc_data_channel_->Send(buffer));

  provider_.set_send_blocked(false);
  SetChannelReady();
  EXPECT_EQ(0U, webrtc_data_channel_->buffered_amount());
}

// Tests that the queued control message is sent when channel is ready.
TEST_F(SctpDataChannelTest, OpenMessageSent) {
  // Initially the id is unassigned.
  EXPECT_EQ(-1, webrtc_data_channel_->id());

  SetChannelReady();
  EXPECT_GE(webrtc_data_channel_->id(), 0);
  EXPECT_EQ(cricket::DMT_CONTROL, provider_.last_send_data_params().type);
  EXPECT_EQ(provider_.last_send_data_params().ssrc,
            static_cast<uint32>(webrtc_data_channel_->id()));
}

// Tests that the DataChannel created after transport gets ready can enter OPEN
// state.
TEST_F(SctpDataChannelTest, LateCreatedChannelTransitionToOpen) {
  SetChannelReady();
  webrtc::InternalDataChannelInit init;
  init.id = 1;
  talk_base::scoped_refptr<DataChannel> dc = DataChannel::Create(
      &provider_, cricket::DCT_SCTP, "test1", init);
  EXPECT_EQ(webrtc::DataChannelInterface::kConnecting, dc->state());
  EXPECT_TRUE_WAIT(webrtc::DataChannelInterface::kOpen == dc->state(),
                   1000);
}

// Tests that an unordered DataChannel sends data as ordered until the OPEN_ACK
// message is received.
TEST_F(SctpDataChannelTest, SendUnorderedAfterReceivesOpenAck) {
  SetChannelReady();
  webrtc::InternalDataChannelInit init;
  init.id = 1;
  init.ordered = false;
  talk_base::scoped_refptr<DataChannel> dc = DataChannel::Create(
      &provider_, cricket::DCT_SCTP, "test1", init);

  EXPECT_EQ_WAIT(webrtc::DataChannelInterface::kOpen, dc->state(), 1000);

  // Sends a message and verifies it's ordered.
  webrtc::DataBuffer buffer("some data");
  ASSERT_TRUE(dc->Send(buffer));
  EXPECT_TRUE(provider_.last_send_data_params().ordered);

  // Emulates receiving an OPEN_ACK message.
  cricket::ReceiveDataParams params;
  params.ssrc = init.id;
  params.type = cricket::DMT_CONTROL;
  talk_base::Buffer payload;
  webrtc::WriteDataChannelOpenAckMessage(&payload);
  dc->OnDataReceived(NULL, params, payload);

  // Sends another message and verifies it's unordered.
  ASSERT_TRUE(dc->Send(buffer));
  EXPECT_FALSE(provider_.last_send_data_params().ordered);
}

// Tests that an unordered DataChannel sends unordered data after any DATA
// message is received.
TEST_F(SctpDataChannelTest, SendUnorderedAfterReceiveData) {
  SetChannelReady();
  webrtc::InternalDataChannelInit init;
  init.id = 1;
  init.ordered = false;
  talk_base::scoped_refptr<DataChannel> dc = DataChannel::Create(
      &provider_, cricket::DCT_SCTP, "test1", init);

  EXPECT_EQ_WAIT(webrtc::DataChannelInterface::kOpen, dc->state(), 1000);

  // Emulates receiving a DATA message.
  cricket::ReceiveDataParams params;
  params.ssrc = init.id;
  params.type = cricket::DMT_TEXT;
  webrtc::DataBuffer buffer("data");
  dc->OnDataReceived(NULL, params, buffer.data);

  // Sends a message and verifies it's unordered.
  ASSERT_TRUE(dc->Send(buffer));
  EXPECT_FALSE(provider_.last_send_data_params().ordered);
}

// Tests that messages are sent with the right ssrc.
TEST_F(SctpDataChannelTest, SendDataSsrc) {
  webrtc_data_channel_->SetSctpSid(1);
  SetChannelReady();
  webrtc::DataBuffer buffer("data");
  EXPECT_TRUE(webrtc_data_channel_->Send(buffer));
  EXPECT_EQ(1U, provider_.last_send_data_params().ssrc);
}

// Tests that the incoming messages with wrong ssrcs are rejected.
TEST_F(SctpDataChannelTest, ReceiveDataWithInvalidSsrc) {
  webrtc_data_channel_->SetSctpSid(1);
  SetChannelReady();

  AddObserver();

  cricket::ReceiveDataParams params;
  params.ssrc = 0;
  webrtc::DataBuffer buffer("abcd");
  webrtc_data_channel_->OnDataReceived(NULL, params, buffer.data);

  EXPECT_EQ(0U, observer_->messages_received());
}

// Tests that the incoming messages with right ssrcs are acceted.
TEST_F(SctpDataChannelTest, ReceiveDataWithValidSsrc) {
  webrtc_data_channel_->SetSctpSid(1);
  SetChannelReady();

  AddObserver();

  cricket::ReceiveDataParams params;
  params.ssrc = 1;
  webrtc::DataBuffer buffer("abcd");

  webrtc_data_channel_->OnDataReceived(NULL, params, buffer.data);
  EXPECT_EQ(1U, observer_->messages_received());
}

// Tests that no CONTROL message is sent if the datachannel is negotiated and
// not created from an OPEN message.
TEST_F(SctpDataChannelTest, NoMsgSentIfNegotiatedAndNotFromOpenMsg) {
  webrtc::InternalDataChannelInit config;
  config.id = 1;
  config.negotiated = true;
  config.open_handshake_role = webrtc::InternalDataChannelInit::kNone;

  SetChannelReady();
  talk_base::scoped_refptr<DataChannel> dc = DataChannel::Create(
      &provider_, cricket::DCT_SCTP, "test1", config);

  EXPECT_EQ_WAIT(webrtc::DataChannelInterface::kOpen, dc->state(), 1000);
  EXPECT_EQ(0U, provider_.last_send_data_params().ssrc);
}

// Tests that OPEN_ACK message is sent if the datachannel is created from an
// OPEN message.
TEST_F(SctpDataChannelTest, OpenAckSentIfCreatedFromOpenMessage) {
  webrtc::InternalDataChannelInit config;
  config.id = 1;
  config.negotiated = true;
  config.open_handshake_role = webrtc::InternalDataChannelInit::kAcker;

  SetChannelReady();
  talk_base::scoped_refptr<DataChannel> dc = DataChannel::Create(
      &provider_, cricket::DCT_SCTP, "test1", config);

  EXPECT_EQ_WAIT(webrtc::DataChannelInterface::kOpen, dc->state(), 1000);

  EXPECT_EQ(static_cast<unsigned int>(config.id),
            provider_.last_send_data_params().ssrc);
  EXPECT_EQ(cricket::DMT_CONTROL, provider_.last_send_data_params().type);
}

// Tests the OPEN_ACK role assigned by InternalDataChannelInit.
TEST_F(SctpDataChannelTest, OpenAckRoleInitialization) {
  webrtc::InternalDataChannelInit init;
  EXPECT_EQ(webrtc::InternalDataChannelInit::kOpener, init.open_handshake_role);
  EXPECT_FALSE(init.negotiated);

  webrtc::DataChannelInit base;
  base.negotiated = true;
  webrtc::InternalDataChannelInit init2(base);
  EXPECT_EQ(webrtc::InternalDataChannelInit::kNone, init2.open_handshake_role);
}

// Tests that the DataChannel is closed if the sending buffer is full.
TEST_F(SctpDataChannelTest, ClosedWhenSendBufferFull) {
  SetChannelReady();
  webrtc::DataBuffer buffer("abcd");
  provider_.set_send_blocked(true);

  for (size_t i = 0; i < 101; ++i) {
    EXPECT_TRUE(webrtc_data_channel_->Send(buffer));
  }

  EXPECT_EQ(webrtc::DataChannelInterface::kClosed,
            webrtc_data_channel_->state());
}

// Tests that the DataChannel is closed on transport errors.
TEST_F(SctpDataChannelTest, ClosedOnTransportError) {
  SetChannelReady();
  webrtc::DataBuffer buffer("abcd");
  provider_.set_transport_error();

  EXPECT_TRUE(webrtc_data_channel_->Send(buffer));

  EXPECT_EQ(webrtc::DataChannelInterface::kClosed,
            webrtc_data_channel_->state());
}

// Tests that a already closed DataChannel does not fire onStateChange again.
TEST_F(SctpDataChannelTest, ClosedDataChannelDoesNotFireOnStateChange) {
  AddObserver();
  webrtc_data_channel_->Close();
  // OnStateChange called for kClosing and kClosed.
  EXPECT_EQ(2U, observer_->on_state_change_count());

  observer_->ResetOnStateChangeCount();
  webrtc_data_channel_->RemotePeerRequestClose();
  EXPECT_EQ(0U, observer_->on_state_change_count());
}

// Tests that RemotePeerRequestClose closes the local DataChannel.
TEST_F(SctpDataChannelTest, RemotePeerRequestClose) {
  AddObserver();
  webrtc_data_channel_->RemotePeerRequestClose();

  // OnStateChange called for kClosing and kClosed.
  EXPECT_EQ(2U, observer_->on_state_change_count());
  EXPECT_EQ(webrtc::DataChannelInterface::kClosed,
            webrtc_data_channel_->state());
}

