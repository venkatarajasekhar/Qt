/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_TEST_TESTSUPPORT_PACKET_READER_H_
#define WEBRTC_TEST_TESTSUPPORT_PACKET_READER_H_

#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {

// Reads chunks of data to simulate network packets from a byte array.
class PacketReader {
 public:
  PacketReader();
  virtual ~PacketReader();

  // Inizializes a new reading operation. Must be done before invoking the
  // NextPacket method.
  // * data_length_in_bytes is the length of the data byte array. Must be >= 0.
  //   0 length will result in no packets are read.
  // * packet_size_in_bytes is the number of bytes to read in each NextPacket
  //   method call. Must be > 0
  virtual void InitializeReading(uint8_t* data, int data_length_in_bytes,
                                 int packet_size_in_bytes);

  // Moves the supplied pointer to the beginning of the next packet.
  // Returns:
  // *  The size of the packet ready to read (lower than the packet size for
  //    the last packet)
  // *  0 if there are no more packets to read
  // * -1 if InitializeReading has not been called (also prints to stderr).
  virtual int NextPacket(uint8_t** packet_pointer);

 private:
  uint8_t* data_;
  int data_length_;
  int packet_size_;
  int currentIndex_;
  bool initialized_;
};

}  // namespace test
}  // namespace webrtc

#endif  // WEBRTC_TEST_TESTSUPPORT_PACKET_READER_H_
