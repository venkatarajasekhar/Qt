// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The entity that handles framing writes for a Quic client or server.
// Each QuicSession will have a connection associated with it.
//
// On the server side, the Dispatcher handles the raw reads, and hands off
// packets via ProcessUdpPacket for framing and processing.
//
// On the client side, the Connection handles the raw reads, as well as the
// processing.
//
// Note: this class is not thread-safe.

#ifndef NET_QUIC_QUIC_CONNECTION_H_
#define NET_QUIC_QUIC_CONNECTION_H_

#include <stddef.h>
#include <deque>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "base/logging.h"
#include "net/base/iovec.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/iovector.h"
#include "net/quic/quic_ack_notifier.h"
#include "net/quic/quic_ack_notifier_manager.h"
#include "net/quic/quic_alarm.h"
#include "net/quic/quic_blocked_writer_interface.h"
#include "net/quic/quic_connection_stats.h"
#include "net/quic/quic_packet_creator.h"
#include "net/quic/quic_packet_generator.h"
#include "net/quic/quic_packet_writer.h"
#include "net/quic/quic_protocol.h"
#include "net/quic/quic_received_packet_manager.h"
#include "net/quic/quic_sent_entropy_manager.h"
#include "net/quic/quic_sent_packet_manager.h"
#include "net/quic/quic_types.h"

namespace net {

class QuicClock;
class QuicConfig;
class QuicConnection;
class QuicDecrypter;
class QuicEncrypter;
class QuicFecGroup;
class QuicRandom;

namespace test {
class QuicConnectionPeer;
}  // namespace test

// Class that receives callbacks from the connection when frames are received
// and when other interesting events happen.
class NET_EXPORT_PRIVATE QuicConnectionVisitorInterface {
 public:
  virtual ~QuicConnectionVisitorInterface() {}

  // A simple visitor interface for dealing with data frames.
  virtual void OnStreamFrames(const std::vector<QuicStreamFrame>& frames) = 0;

  // The session should process all WINDOW_UPDATE frames, adjusting both stream
  // and connection level flow control windows.
  virtual void OnWindowUpdateFrames(
      const std::vector<QuicWindowUpdateFrame>& frames) = 0;

  // BLOCKED frames tell us that the peer believes it is flow control blocked on
  // a specified stream. If the session at this end disagrees, something has
  // gone wrong with our flow control accounting.
  virtual void OnBlockedFrames(const std::vector<QuicBlockedFrame>& frames) = 0;

  // Called when the stream is reset by the peer.
  virtual void OnRstStream(const QuicRstStreamFrame& frame) = 0;

  // Called when the connection is going away according to the peer.
  virtual void OnGoAway(const QuicGoAwayFrame& frame) = 0;

  // Called when the connection is closed either locally by the framer, or
  // remotely by the peer.
  virtual void OnConnectionClosed(QuicErrorCode error, bool from_peer) = 0;

  // Called when the connection failed to write because the socket was blocked.
  virtual void OnWriteBlocked() = 0;

  // Called once a specific QUIC version is agreed by both endpoints.
  virtual void OnSuccessfulVersionNegotiation(const QuicVersion& version) = 0;

  // Called when a blocked socket becomes writable.
  virtual void OnCanWrite() = 0;

  // Called to ask if the visitor wants to schedule write resumption as it both
  // has pending data to write, and is able to write (e.g. based on flow control
  // limits).
  // Writes may be pending because they were write-blocked, congestion-throttled
  // or yielded to other connections.
  virtual bool WillingAndAbleToWrite() const = 0;

  // Called to ask if any handshake messages are pending in this visitor.
  virtual bool HasPendingHandshake() const = 0;

  // Called to ask if any streams are open in this visitor, excluding the
  // reserved crypto and headers stream.
  virtual bool HasOpenDataStreams() const = 0;
};

// Interface which gets callbacks from the QuicConnection at interesting
// points.  Implementations must not mutate the state of the connection
// as a result of these callbacks.
class NET_EXPORT_PRIVATE QuicConnectionDebugVisitor
    : public QuicPacketGenerator::DebugDelegate,
      public QuicSentPacketManager::DebugDelegate {
 public:
  virtual ~QuicConnectionDebugVisitor() {}

  // Called when a packet has been sent.
  virtual void OnPacketSent(QuicPacketSequenceNumber sequence_number,
                            EncryptionLevel level,
                            TransmissionType transmission_type,
                            const QuicEncryptedPacket& packet,
                            WriteResult result) {}

  // Called when the contents of a packet have been retransmitted as
  // a new packet.
  virtual void OnPacketRetransmitted(
      QuicPacketSequenceNumber old_sequence_number,
      QuicPacketSequenceNumber new_sequence_number) {}

  // Called when a packet has been received, but before it is
  // validated or parsed.
  virtual void OnPacketReceived(const IPEndPoint& self_address,
                                const IPEndPoint& peer_address,
                                const QuicEncryptedPacket& packet) {}

  // Called when the protocol version on the received packet doensn't match
  // current protocol version of the connection.
  virtual void OnProtocolVersionMismatch(QuicVersion version) {}

  // Called when the complete header of a packet has been parsed.
  virtual void OnPacketHeader(const QuicPacketHeader& header) {}

  // Called when a StreamFrame has been parsed.
  virtual void OnStreamFrame(const QuicStreamFrame& frame) {}

  // Called when a AckFrame has been parsed.
  virtual void OnAckFrame(const QuicAckFrame& frame) {}

  // Called when a CongestionFeedbackFrame has been parsed.
  virtual void OnCongestionFeedbackFrame(
      const QuicCongestionFeedbackFrame& frame) {}

  // Called when a StopWaitingFrame has been parsed.
  virtual void OnStopWaitingFrame(const QuicStopWaitingFrame& frame) {}

  // Called when a Ping has been parsed.
  virtual void OnPingFrame(const QuicPingFrame& frame) {}

  // Called when a GoAway has been parsed.
  virtual void OnGoAwayFrame(const QuicGoAwayFrame& frame) {}

  // Called when a RstStreamFrame has been parsed.
  virtual void OnRstStreamFrame(const QuicRstStreamFrame& frame) {}

  // Called when a ConnectionCloseFrame has been parsed.
  virtual void OnConnectionCloseFrame(
      const QuicConnectionCloseFrame& frame) {}

  // Called when a WindowUpdate has been parsed.
  virtual void OnWindowUpdateFrame(const QuicWindowUpdateFrame& frame) {}

  // Called when a BlockedFrame has been parsed.
  virtual void OnBlockedFrame(const QuicBlockedFrame& frame) {}

  // Called when a public reset packet has been received.
  virtual void OnPublicResetPacket(const QuicPublicResetPacket& packet) {}

  // Called when a version negotiation packet has been received.
  virtual void OnVersionNegotiationPacket(
      const QuicVersionNegotiationPacket& packet) {}

  // Called after a packet has been successfully parsed which results
  // in the revival of a packet via FEC.
  virtual void OnRevivedPacket(const QuicPacketHeader& revived_header,
                               base::StringPiece payload) {}
};

class NET_EXPORT_PRIVATE QuicConnectionHelperInterface {
 public:
  virtual ~QuicConnectionHelperInterface() {}

  // Returns a QuicClock to be used for all time related functions.
  virtual const QuicClock* GetClock() const = 0;

  // Returns a QuicRandom to be used for all random number related functions.
  virtual QuicRandom* GetRandomGenerator() = 0;

  // Creates a new platform-specific alarm which will be configured to
  // notify |delegate| when the alarm fires.  Caller takes ownership
  // of the new alarm, which will not yet be "set" to fire.
  virtual QuicAlarm* CreateAlarm(QuicAlarm::Delegate* delegate) = 0;
};

class NET_EXPORT_PRIVATE QuicConnection
    : public QuicFramerVisitorInterface,
      public QuicBlockedWriterInterface,
      public QuicPacketGenerator::DelegateInterface {
 public:
  enum PacketType {
    NORMAL,
    QUEUED,
    CONNECTION_CLOSE
  };

  enum AckBundling {
    NO_ACK = 0,
    SEND_ACK = 1,
    BUNDLE_PENDING_ACK = 2,
  };

  // Constructs a new QuicConnection for |connection_id| and |address|.
  // |helper| and |writer| must outlive this connection.
  QuicConnection(QuicConnectionId connection_id,
                 IPEndPoint address,
                 QuicConnectionHelperInterface* helper,
                 QuicPacketWriter* writer,
                 bool is_server,
                 const QuicVersionVector& supported_versions);
  virtual ~QuicConnection();

  // Sets connection parameters from the supplied |config|.
  void SetFromConfig(const QuicConfig& config);

  // Send the data in |data| to the peer in as few packets as possible.
  // Returns a pair with the number of bytes consumed from data, and a boolean
  // indicating if the fin bit was consumed.  This does not indicate the data
  // has been sent on the wire: it may have been turned into a packet and queued
  // if the socket was unexpectedly blocked. |fec_protection| indicates if
  // data is to be FEC protected. Note that data that is sent immediately
  // following MUST_FEC_PROTECT data may get protected by falling within the
  // same FEC group.
  // If |delegate| is provided, then it will be informed once ACKs have been
  // received for all the packets written in this call.
  // The |delegate| is not owned by the QuicConnection and must outlive it.
  QuicConsumedData SendStreamData(QuicStreamId id,
                                  const IOVector& data,
                                  QuicStreamOffset offset,
                                  bool fin,
                                  FecProtection fec_protection,
                                  QuicAckNotifier::DelegateInterface* delegate);

  // Send a RST_STREAM frame to the peer.
  virtual void SendRstStream(QuicStreamId id,
                             QuicRstStreamErrorCode error,
                             QuicStreamOffset bytes_written);

  // Send a BLOCKED frame to the peer.
  virtual void SendBlocked(QuicStreamId id);

  // Send a WINDOW_UPDATE frame to the peer.
  virtual void SendWindowUpdate(QuicStreamId id,
                                QuicStreamOffset byte_offset);

  // Sends the connection close packet without affecting the state of the
  // connection.  This should only be called if the session is actively being
  // destroyed: otherwise call SendConnectionCloseWithDetails instead.
  virtual void SendConnectionClosePacket(QuicErrorCode error,
                                         const std::string& details);

  // Sends a connection close frame to the peer, and closes the connection by
  // calling CloseConnection(notifying the visitor as it does so).
  virtual void SendConnectionClose(QuicErrorCode error);
  virtual void SendConnectionCloseWithDetails(QuicErrorCode error,
                                              const std::string& details);
  // Notifies the visitor of the close and marks the connection as disconnected.
  virtual void CloseConnection(QuicErrorCode error, bool from_peer) OVERRIDE;
  virtual void SendGoAway(QuicErrorCode error,
                          QuicStreamId last_good_stream_id,
                          const std::string& reason);

  // Returns statistics tracked for this connection.
  const QuicConnectionStats& GetStats();

  // Processes an incoming UDP packet (consisting of a QuicEncryptedPacket) from
  // the peer.  If processing this packet permits a packet to be revived from
  // its FEC group that packet will be revived and processed.
  virtual void ProcessUdpPacket(const IPEndPoint& self_address,
                                const IPEndPoint& peer_address,
                                const QuicEncryptedPacket& packet);

  // QuicBlockedWriterInterface
  // Called when the underlying connection becomes writable to allow queued
  // writes to happen.
  virtual void OnCanWrite() OVERRIDE;

  // Called when a packet has been finally sent to the network.
  bool OnPacketSent(WriteResult result);

  // If the socket is not blocked, writes queued packets.
  void WriteIfNotBlocked();

  // Do any work which logically would be done in OnPacket but can not be
  // safely done until the packet is validated.  Returns true if the packet
  // can be handled, false otherwise.
  bool ProcessValidatedPacket();

  // The version of the protocol this connection is using.
  QuicVersion version() const { return framer_.version(); }

  // The versions of the protocol that this connection supports.
  const QuicVersionVector& supported_versions() const {
    return framer_.supported_versions();
  }

  // From QuicFramerVisitorInterface
  virtual void OnError(QuicFramer* framer) OVERRIDE;
  virtual bool OnProtocolVersionMismatch(QuicVersion received_version) OVERRIDE;
  virtual void OnPacket() OVERRIDE;
  virtual void OnPublicResetPacket(
      const QuicPublicResetPacket& packet) OVERRIDE;
  virtual void OnVersionNegotiationPacket(
      const QuicVersionNegotiationPacket& packet) OVERRIDE;
  virtual void OnRevivedPacket() OVERRIDE;
  virtual bool OnUnauthenticatedPublicHeader(
      const QuicPacketPublicHeader& header) OVERRIDE;
  virtual bool OnUnauthenticatedHeader(const QuicPacketHeader& header) OVERRIDE;
  virtual void OnDecryptedPacket(EncryptionLevel level) OVERRIDE;
  virtual bool OnPacketHeader(const QuicPacketHeader& header) OVERRIDE;
  virtual void OnFecProtectedPayload(base::StringPiece payload) OVERRIDE;
  virtual bool OnStreamFrame(const QuicStreamFrame& frame) OVERRIDE;
  virtual bool OnAckFrame(const QuicAckFrame& frame) OVERRIDE;
  virtual bool OnCongestionFeedbackFrame(
      const QuicCongestionFeedbackFrame& frame) OVERRIDE;
  virtual bool OnStopWaitingFrame(const QuicStopWaitingFrame& frame) OVERRIDE;
  virtual bool OnPingFrame(const QuicPingFrame& frame) OVERRIDE;
  virtual bool OnRstStreamFrame(const QuicRstStreamFrame& frame) OVERRIDE;
  virtual bool OnConnectionCloseFrame(
      const QuicConnectionCloseFrame& frame) OVERRIDE;
  virtual bool OnGoAwayFrame(const QuicGoAwayFrame& frame) OVERRIDE;
  virtual bool OnWindowUpdateFrame(const QuicWindowUpdateFrame& frame) OVERRIDE;
  virtual bool OnBlockedFrame(const QuicBlockedFrame& frame) OVERRIDE;
  virtual void OnFecData(const QuicFecData& fec) OVERRIDE;
  virtual void OnPacketComplete() OVERRIDE;

  // QuicPacketGenerator::DelegateInterface
  virtual bool ShouldGeneratePacket(TransmissionType transmission_type,
                                    HasRetransmittableData retransmittable,
                                    IsHandshake handshake) OVERRIDE;
  virtual QuicAckFrame* CreateAckFrame() OVERRIDE;
  virtual QuicCongestionFeedbackFrame* CreateFeedbackFrame() OVERRIDE;
  virtual QuicStopWaitingFrame* CreateStopWaitingFrame() OVERRIDE;
  virtual bool OnSerializedPacket(const SerializedPacket& packet) OVERRIDE;

  // Accessors
  void set_visitor(QuicConnectionVisitorInterface* visitor) {
    visitor_ = visitor;
  }
  void set_debug_visitor(QuicConnectionDebugVisitor* debug_visitor) {
    debug_visitor_ = debug_visitor;
    packet_generator_.set_debug_delegate(debug_visitor);
    sent_packet_manager_.set_debug_delegate(debug_visitor);
  }
  const IPEndPoint& self_address() const { return self_address_; }
  const IPEndPoint& peer_address() const { return peer_address_; }
  QuicConnectionId connection_id() const { return connection_id_; }
  const QuicClock* clock() const { return clock_; }
  QuicRandom* random_generator() const { return random_generator_; }
  size_t max_packet_length() const;
  void set_max_packet_length(size_t length);

  bool connected() const { return connected_; }

  // Must only be called on client connections.
  const QuicVersionVector& server_supported_versions() const {
    DCHECK(!is_server_);
    return server_supported_versions_;
  }

  size_t NumFecGroups() const { return group_map_.size(); }

  // Testing only.
  size_t NumQueuedPackets() const { return queued_packets_.size(); }

  QuicEncryptedPacket* ReleaseConnectionClosePacket() {
    return connection_close_packet_.release();
  }

  // Returns true if the underlying UDP socket is writable, there is
  // no queued data and the connection is not congestion-control
  // blocked.
  bool CanWriteStreamData();

  // Returns true if the connection has queued packets or frames.
  bool HasQueuedData() const;

  // Sets (or resets) the idle state connection timeout. Also, checks and times
  // out the connection if network timer has expired for |timeout|.
  void SetIdleNetworkTimeout(QuicTime::Delta timeout);
  // Sets (or resets) the total time delta the connection can be alive for.
  // Also, checks and times out the connection if timer has expired for
  // |timeout|. Used to limit the time a connection can be alive before crypto
  // handshake finishes.
  void SetOverallConnectionTimeout(QuicTime::Delta timeout);

  // If the connection has timed out, this will close the connection and return
  // true.  Otherwise, it will return false and will reset the timeout alarm.
  bool CheckForTimeout();

  // Sends a ping, and resets the ping alarm.
  void SendPing();

  // Sets up a packet with an QuicAckFrame and sends it out.
  void SendAck();

  // Called when an RTO fires.  Resets the retransmission alarm if there are
  // remaining unacked packets.
  void OnRetransmissionTimeout();

  // Retransmits all unacked packets with retransmittable frames if
  // |retransmission_type| is ALL_PACKETS, otherwise retransmits only initially
  // encrypted packets. Used when the negotiated protocol version is different
  // from what was initially assumed and when the visitor wants to re-transmit
  // initially encrypted packets when the initial encrypter changes.
  void RetransmitUnackedPackets(RetransmissionType retransmission_type);

  // Calls |sent_packet_manager_|'s NeuterUnencryptedPackets. Used when the
  // connection becomes forward secure and hasn't received acks for all packets.
  void NeuterUnencryptedPackets();

  // Changes the encrypter used for level |level| to |encrypter|. The function
  // takes ownership of |encrypter|.
  void SetEncrypter(EncryptionLevel level, QuicEncrypter* encrypter);
  const QuicEncrypter* encrypter(EncryptionLevel level) const;

  // SetDefaultEncryptionLevel sets the encryption level that will be applied
  // to new packets.
  void SetDefaultEncryptionLevel(EncryptionLevel level);

  // SetDecrypter sets the primary decrypter, replacing any that already exists,
  // and takes ownership. If an alternative decrypter is in place then the
  // function DCHECKs. This is intended for cases where one knows that future
  // packets will be using the new decrypter and the previous decrypter is now
  // obsolete. |level| indicates the encryption level of the new decrypter.
  void SetDecrypter(QuicDecrypter* decrypter, EncryptionLevel level);

  // SetAlternativeDecrypter sets a decrypter that may be used to decrypt
  // future packets and takes ownership of it. |level| indicates the encryption
  // level of the decrypter. If |latch_once_used| is true, then the first time
  // that the decrypter is successful it will replace the primary decrypter.
  // Otherwise both decrypters will remain active and the primary decrypter
  // will be the one last used.
  void SetAlternativeDecrypter(QuicDecrypter* decrypter,
                               EncryptionLevel level,
                               bool latch_once_used);

  const QuicDecrypter* decrypter() const;
  const QuicDecrypter* alternative_decrypter() const;

  bool is_server() const { return is_server_; }

  // Returns the underlying sent packet manager.
  const QuicSentPacketManager& sent_packet_manager() const {
    return sent_packet_manager_;
  }

  bool CanWrite(HasRetransmittableData retransmittable);

  // Stores current batch state for connection, puts the connection
  // into batch mode, and destruction restores the stored batch state.
  // While the bundler is in scope, any generated frames are bundled
  // as densely as possible into packets.  In addition, this bundler
  // can be configured to ensure that an ACK frame is included in the
  // first packet created, if there's new ack information to be sent.
  class ScopedPacketBundler {
   public:
    // In addition to all outgoing frames being bundled when the
    // bundler is in scope, setting |include_ack| to true ensures that
    // an ACK frame is opportunistically bundled with the first
    // outgoing packet.
    ScopedPacketBundler(QuicConnection* connection, AckBundling send_ack);
    ~ScopedPacketBundler();

   private:
    QuicConnection* connection_;
    bool already_in_batch_mode_;
  };

 protected:
  // Send a packet to the peer using encryption |level|. If |sequence_number|
  // is present in the |retransmission_map_|, then contents of this packet will
  // be retransmitted with a new sequence number if it's not acked by the peer.
  // Deletes |packet| if WritePacket call succeeds, or transfers ownership to
  // QueuedPacket, ultimately deleted in WriteQueuedPackets. Updates the
  // entropy map corresponding to |sequence_number| using |entropy_hash|.
  // |transmission_type| and |retransmittable| are supplied to the congestion
  // manager, and when |forced| is true, it bypasses the congestion manager.
  // TODO(wtc): none of the callers check the return value.
  virtual bool SendOrQueuePacket(EncryptionLevel level,
                                 const SerializedPacket& packet,
                                 TransmissionType transmission_type);

  QuicConnectionHelperInterface* helper() { return helper_; }

  // Selects and updates the version of the protocol being used by selecting a
  // version from |available_versions| which is also supported. Returns true if
  // such a version exists, false otherwise.
  bool SelectMutualVersion(const QuicVersionVector& available_versions);

  QuicPacketWriter* writer() { return writer_; }

 private:
  friend class test::QuicConnectionPeer;

  // Packets which have not been written to the wire.
  // Owns the QuicPacket* packet.
  struct QueuedPacket {
    QueuedPacket(SerializedPacket packet,
                 EncryptionLevel level,
                 TransmissionType transmission_type);

    QuicPacketSequenceNumber sequence_number;
    QuicPacket* packet;
    const EncryptionLevel encryption_level;
    TransmissionType transmission_type;
    HasRetransmittableData retransmittable;
    IsHandshake handshake;
    PacketType type;
    QuicByteCount length;
  };

  typedef std::list<QueuedPacket> QueuedPacketList;
  typedef std::map<QuicFecGroupNumber, QuicFecGroup*> FecGroupMap;

  // Writes the given packet to socket, encrypted with packet's
  // encryption_level. Returns true on successful write. Behavior is undefined
  // if connection is not established or broken. A return value of true means
  // the packet was transmitted and may be destroyed. If the packet is
  // retransmittable, WritePacket sets up retransmission for a successful write.
  // If packet is FORCE, then it will be sent immediately and the send scheduler
  // will not be consulted.
  bool WritePacket(QueuedPacket packet);

  // Make sure an ack we got from our peer is sane.
  bool ValidateAckFrame(const QuicAckFrame& incoming_ack);

  // Make sure a stop waiting we got from our peer is sane.
  bool ValidateStopWaitingFrame(const QuicStopWaitingFrame& stop_waiting);

  // Sends a version negotiation packet to the peer.
  void SendVersionNegotiationPacket();

  // Clears any accumulated frames from the last received packet.
  void ClearLastFrames();

  // Writes as many queued packets as possible.  The connection must not be
  // blocked when this is called.
  void WriteQueuedPackets();

  // Writes as many pending retransmissions as possible.
  void WritePendingRetransmissions();

  // Returns true if the packet should be discarded and not sent.
  bool ShouldDiscardPacket(EncryptionLevel level,
                           QuicPacketSequenceNumber sequence_number,
                           HasRetransmittableData retransmittable);

  // Queues |packet| in the hopes that it can be decrypted in the
  // future, when a new key is installed.
  void QueueUndecryptablePacket(const QuicEncryptedPacket& packet);

  // Attempts to process any queued undecryptable packets.
  void MaybeProcessUndecryptablePackets();

  // If a packet can be revived from the current FEC group, then
  // revive and process the packet.
  void MaybeProcessRevivedPacket();

  void ProcessAckFrame(const QuicAckFrame& incoming_ack);

  void ProcessStopWaitingFrame(const QuicStopWaitingFrame& stop_waiting);

  // Update |stop_waiting| for an outgoing ack.
  void UpdateStopWaiting(QuicStopWaitingFrame* stop_waiting);

  // Queues an ack or sets the ack alarm when an incoming packet arrives that
  // should be acked.
  void MaybeQueueAck();

  // Checks if the last packet should instigate an ack.
  bool ShouldLastPacketInstigateAck() const;

  // Checks if the peer is waiting for packets that have been given up on, and
  // therefore an ack frame should be sent with a larger least_unacked.
  void UpdateStopWaitingCount();

  // Sends any packets which are a response to the last packet, including both
  // acks and pending writes if an ack opened the congestion window.
  void MaybeSendInResponseToPacket();

  // Gets the least unacked sequence number, which is the next sequence number
  // to be sent if there are no outstanding packets.
  QuicPacketSequenceNumber GetLeastUnacked() const;

  // Get the FEC group associate with the last processed packet or NULL, if the
  // group has already been deleted.
  QuicFecGroup* GetFecGroup();

  // Closes any FEC groups protecting packets before |sequence_number|.
  void CloseFecGroupsBefore(QuicPacketSequenceNumber sequence_number);

  // Sets the ping alarm to the appropriate value, if any.
  void SetPingAlarm();

  // On arrival of a new packet, checks to see if the socket addresses have
  // changed since the last packet we saw on this connection.
  void CheckForAddressMigration(const IPEndPoint& self_address,
                                const IPEndPoint& peer_address);

  QuicFramer framer_;
  QuicConnectionHelperInterface* helper_;  // Not owned.
  QuicPacketWriter* writer_;  // Not owned.
  EncryptionLevel encryption_level_;
  const QuicClock* clock_;
  QuicRandom* random_generator_;

  const QuicConnectionId connection_id_;
  // Address on the last successfully processed packet received from the
  // client.
  IPEndPoint self_address_;
  IPEndPoint peer_address_;
  // Used to store latest peer port to possibly migrate to later.
  int migrating_peer_port_;

  bool last_packet_revived_;  // True if the last packet was revived from FEC.
  size_t last_size_;  // Size of the last received packet.
  EncryptionLevel last_decrypted_packet_level_;
  QuicPacketHeader last_header_;
  std::vector<QuicStreamFrame> last_stream_frames_;
  std::vector<QuicAckFrame> last_ack_frames_;
  std::vector<QuicCongestionFeedbackFrame> last_congestion_frames_;
  std::vector<QuicStopWaitingFrame> last_stop_waiting_frames_;
  std::vector<QuicRstStreamFrame> last_rst_frames_;
  std::vector<QuicGoAwayFrame> last_goaway_frames_;
  std::vector<QuicWindowUpdateFrame> last_window_update_frames_;
  std::vector<QuicBlockedFrame> last_blocked_frames_;
  std::vector<QuicConnectionCloseFrame> last_close_frames_;

  QuicCongestionFeedbackFrame outgoing_congestion_feedback_;

  // Track some peer state so we can do less bookkeeping
  // Largest sequence sent by the peer which had an ack frame (latest ack info).
  QuicPacketSequenceNumber largest_seen_packet_with_ack_;

  // Largest sequence number sent by the peer which had a stop waiting frame.
  QuicPacketSequenceNumber largest_seen_packet_with_stop_waiting_;

  // Collection of packets which were received before encryption was
  // established, but which could not be decrypted.  We buffer these on
  // the assumption that they could not be processed because they were
  // sent with the INITIAL encryption and the CHLO message was lost.
  std::deque<QuicEncryptedPacket*> undecryptable_packets_;

  // When the version negotiation packet could not be sent because the socket
  // was not writable, this is set to true.
  bool pending_version_negotiation_packet_;

  // When packets could not be sent because the socket was not writable,
  // they are added to this list.  All corresponding frames are in
  // unacked_packets_ if they are to be retransmitted.
  QueuedPacketList queued_packets_;

  // Contains information about the current write in progress, if any.
  scoped_ptr<QueuedPacket> pending_write_;

  // Contains the connection close packet if the connection has been closed.
  scoped_ptr<QuicEncryptedPacket> connection_close_packet_;

  FecGroupMap group_map_;

  QuicReceivedPacketManager received_packet_manager_;
  QuicSentEntropyManager sent_entropy_manager_;

  // Indicates whether an ack should be sent the next time we try to write.
  bool ack_queued_;
  // Indicates how many consecutive times an ack has arrived which indicates
  // the peer needs to stop waiting for some packets.
  int stop_waiting_count_;

  // An alarm that fires when an ACK should be sent to the peer.
  scoped_ptr<QuicAlarm> ack_alarm_;
  // An alarm that fires when a packet needs to be retransmitted.
  scoped_ptr<QuicAlarm> retransmission_alarm_;
  // An alarm that is scheduled when the sent scheduler requires a
  // a delay before sending packets and fires when the packet may be sent.
  scoped_ptr<QuicAlarm> send_alarm_;
  // An alarm that is scheduled when the connection can still write and there
  // may be more data to send.
  scoped_ptr<QuicAlarm> resume_writes_alarm_;
  // An alarm that fires when the connection may have timed out.
  scoped_ptr<QuicAlarm> timeout_alarm_;
  // An alarm that fires when a ping should be sent.
  scoped_ptr<QuicAlarm> ping_alarm_;

  QuicConnectionVisitorInterface* visitor_;
  QuicConnectionDebugVisitor* debug_visitor_;
  QuicPacketGenerator packet_generator_;

  // Network idle time before we kill of this connection.
  QuicTime::Delta idle_network_timeout_;
  // Overall connection timeout.
  QuicTime::Delta overall_connection_timeout_;

  // Statistics for this session.
  QuicConnectionStats stats_;

  // The time that we got a packet for this connection.
  // This is used for timeouts, and does not indicate the packet was processed.
  QuicTime time_of_last_received_packet_;

  // The last time a new (non-retransmitted) packet was sent for this
  // connection.
  QuicTime time_of_last_sent_new_packet_;

  // Sequence number of the last sent packet.  Packets are guaranteed to be sent
  // in sequence number order.
  QuicPacketSequenceNumber sequence_number_of_last_sent_packet_;

  // Sent packet manager which tracks the status of packets sent by this
  // connection and contains the send and receive algorithms to determine when
  // to send packets.
  QuicSentPacketManager sent_packet_manager_;

  // The state of connection in version negotiation finite state machine.
  QuicVersionNegotiationState version_negotiation_state_;

  // Tracks if the connection was created by the server.
  bool is_server_;

  // True by default.  False if we've received or sent an explicit connection
  // close.
  bool connected_;

  // Set to true if the UDP packet headers have a new IP address for the peer.
  // If true, do not perform connection migration.
  bool peer_ip_changed_;

  // Set to true if the UDP packet headers have a new port for the peer.
  // If true, and the IP has not changed, then we can migrate the connection.
  bool peer_port_changed_;

  // Set to true if the UDP packet headers are addressed to a different IP.
  // We do not support connection migration when the self IP changed.
  bool self_ip_changed_;

  // Set to true if the UDP packet headers are addressed to a different port.
  // We do not support connection migration when the self port changed.
  bool self_port_changed_;

  // If non-empty this contains the set of versions received in a
  // version negotiation packet.
  QuicVersionVector server_supported_versions_;

  DISALLOW_COPY_AND_ASSIGN(QuicConnection);
};

}  // namespace net

#endif  // NET_QUIC_QUIC_CONNECTION_H_
