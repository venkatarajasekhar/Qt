// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_RTCP_RECEIVER_RTCP_EVENT_SUBSCRIBER_H_
#define MEDIA_CAST_RTCP_RECEIVER_RTCP_EVENT_SUBSCRIBER_H_

#include <map>

#include "base/threading/thread_checker.h"
#include "media/cast/logging/logging_defines.h"
#include "media/cast/logging/raw_event_subscriber.h"
#include "media/cast/rtcp/rtcp_defines.h"

namespace media {
namespace cast {

// A RawEventSubscriber implementation with the following properties:
// - Only processes raw event types that are relevant for sending from cast
//   receiver to cast sender via RTCP.
// - Captures information to be sent over to RTCP from raw event logs into the
//   more compact RtcpEvent struct.
// - Orders events by RTP timestamp with a multimap.
// - Internally, the map is capped at a maximum size configurable by the caller.
//   The subscriber only keeps the most recent events (determined by RTP
//   timestamp) up to the size limit.
class ReceiverRtcpEventSubscriber : public RawEventSubscriber {
 public:
  typedef std::multimap<RtpTimestamp, RtcpEvent> RtcpEventMultiMap;

  // |max_size_to_retain|: The object will keep up to |max_size_to_retain|
  // events
  // in the map. Once threshold has been reached, an event with the smallest
  // RTP timestamp will be removed.
  // |type|: Determines whether the subscriber will process only audio or video
  // events.
  ReceiverRtcpEventSubscriber(const size_t max_size_to_retain,
      EventMediaType type);

  virtual ~ReceiverRtcpEventSubscriber();

  // RawEventSubscriber implementation.
  virtual void OnReceiveFrameEvent(const FrameEvent& frame_event) OVERRIDE;
  virtual void OnReceivePacketEvent(const PacketEvent& packet_event) OVERRIDE;

  // Assigns events collected to |rtcp_events| and clears them from this
  // object.
  void GetRtcpEventsAndReset(RtcpEventMultiMap* rtcp_events);

 private:
  // If |rtcp_events_.size()| exceeds |max_size_to_retain_|, remove an oldest
  // entry (determined by RTP timestamp) so its size no greater than
  // |max_size_to_retain_|.
  void TruncateMapIfNeeded();

  // Returns |true| if events of |event_type| and |media_type|
  // should be processed.
  bool ShouldProcessEvent(CastLoggingEvent event_type,
      EventMediaType media_type);

  const size_t max_size_to_retain_;
  EventMediaType type_;

  // The key should really be something more than just a RTP timestamp in order
  // to differentiate between video and audio frames, but since the
  // implementation doesn't mix audio and video frame events, RTP timestamp
  // only as key is fine.
  RtcpEventMultiMap rtcp_events_;

  // Ensures methods are only called on the main thread.
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(ReceiverRtcpEventSubscriber);
};

}  // namespace cast
}  // namespace media

#endif  // MEDIA_CAST_RTCP_RECEIVER_RTCP_EVENT_SUBSCRIBER_H_
