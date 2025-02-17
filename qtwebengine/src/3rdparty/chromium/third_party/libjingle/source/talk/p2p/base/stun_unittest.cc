/*
 * libjingle
 * Copyright 2004 Google Inc.
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

#include <string>

#include "talk/base/bytebuffer.h"
#include "talk/base/gunit.h"
#include "talk/base/logging.h"
#include "talk/base/messagedigest.h"
#include "talk/base/scoped_ptr.h"
#include "talk/base/socketaddress.h"
#include "talk/p2p/base/stun.h"

namespace cricket {

class StunTest : public ::testing::Test {
 protected:
  void CheckStunHeader(const StunMessage& msg, StunMessageType expected_type,
                       size_t expected_length) {
    ASSERT_EQ(expected_type, msg.type());
    ASSERT_EQ(expected_length, msg.length());
  }

  void CheckStunTransactionID(const StunMessage& msg,
                              const unsigned char* expectedID, size_t length) {
    ASSERT_EQ(length, msg.transaction_id().size());
    ASSERT_EQ(length == kStunTransactionIdLength + 4, msg.IsLegacy());
    ASSERT_EQ(length == kStunTransactionIdLength, !msg.IsLegacy());
    ASSERT_EQ(0, memcmp(msg.transaction_id().c_str(), expectedID, length));
  }

  void CheckStunAddressAttribute(const StunAddressAttribute* addr,
                                 StunAddressFamily expected_family,
                                 int expected_port,
                                 talk_base::IPAddress expected_address) {
    ASSERT_EQ(expected_family, addr->family());
    ASSERT_EQ(expected_port, addr->port());

    if (addr->family() == STUN_ADDRESS_IPV4) {
      in_addr v4_address = expected_address.ipv4_address();
      in_addr stun_address = addr->ipaddr().ipv4_address();
      ASSERT_EQ(0, memcmp(&v4_address, &stun_address, sizeof(stun_address)));
    } else if (addr->family() == STUN_ADDRESS_IPV6) {
      in6_addr v6_address = expected_address.ipv6_address();
      in6_addr stun_address = addr->ipaddr().ipv6_address();
      ASSERT_EQ(0, memcmp(&v6_address, &stun_address, sizeof(stun_address)));
    } else {
      ASSERT_TRUE(addr->family() == STUN_ADDRESS_IPV6 ||
                  addr->family() == STUN_ADDRESS_IPV4);
    }
  }

  size_t ReadStunMessageTestCase(StunMessage* msg,
                                 const unsigned char* testcase,
                                 size_t size) {
    const char* input = reinterpret_cast<const char*>(testcase);
    talk_base::ByteBuffer buf(input, size);
    if (msg->Read(&buf)) {
      // Returns the size the stun message should report itself as being
      return (size - 20);
    } else {
      return 0;
    }
  }
};


// Sample STUN packets with various attributes
// Gathered by wiresharking pjproject's pjnath test programs
// pjproject available at www.pjsip.org

static const unsigned char kStunMessageWithIPv6MappedAddress[] = {
  0x00, 0x01, 0x00, 0x18,  // message header
  0x21, 0x12, 0xa4, 0x42,  // transaction id
  0x29, 0x1f, 0xcd, 0x7c,
  0xba, 0x58, 0xab, 0xd7,
  0xf2, 0x41, 0x01, 0x00,
  0x00, 0x01, 0x00, 0x14,  // Address type (mapped), length
  0x00, 0x02, 0xb8, 0x81,  // family (IPv6), port
  0x24, 0x01, 0xfa, 0x00,  // an IPv6 address
  0x00, 0x04, 0x10, 0x00,
  0xbe, 0x30, 0x5b, 0xff,
  0xfe, 0xe5, 0x00, 0xc3
};

static const unsigned char kStunMessageWithIPv4MappedAddress[] = {
  0x01, 0x01, 0x00, 0x0c,   // binding response, length 12
  0x21, 0x12, 0xa4, 0x42,   // magic cookie
  0x29, 0x1f, 0xcd, 0x7c,   // transaction ID
  0xba, 0x58, 0xab, 0xd7,
  0xf2, 0x41, 0x01, 0x00,
  0x00, 0x01, 0x00, 0x08,  // Mapped, 8 byte length
  0x00, 0x01, 0x9d, 0xfc,  // AF_INET, unxor-ed port
  0xac, 0x17, 0x44, 0xe6   // IPv4 address
};

// Test XOR-mapped IP addresses:
static const unsigned char kStunMessageWithIPv6XorMappedAddress[] = {
  0x01, 0x01, 0x00, 0x18,  // message header (binding response)
  0x21, 0x12, 0xa4, 0x42,  // magic cookie (rfc5389)
  0xe3, 0xa9, 0x46, 0xe1,  // transaction ID
  0x7c, 0x00, 0xc2, 0x62,
  0x54, 0x08, 0x01, 0x00,
  0x00, 0x20, 0x00, 0x14,  // Address Type (XOR), length
  0x00, 0x02, 0xcb, 0x5b,  // family, XOR-ed port
  0x05, 0x13, 0x5e, 0x42,  // XOR-ed IPv6 address
  0xe3, 0xad, 0x56, 0xe1,
  0xc2, 0x30, 0x99, 0x9d,
  0xaa, 0xed, 0x01, 0xc3
};

static const unsigned char kStunMessageWithIPv4XorMappedAddress[] = {
  0x01, 0x01, 0x00, 0x0c,  // message header (binding response)
  0x21, 0x12, 0xa4, 0x42,  // magic cookie
  0x29, 0x1f, 0xcd, 0x7c,  // transaction ID
  0xba, 0x58, 0xab, 0xd7,
  0xf2, 0x41, 0x01, 0x00,
  0x00, 0x20, 0x00, 0x08,  // address type (xor), length
  0x00, 0x01, 0xfc, 0xb5,  // family (AF_INET), XOR-ed port
  0x8d, 0x05, 0xe0, 0xa4   // IPv4 address
};

// ByteString Attribute (username)
static const unsigned char kStunMessageWithByteStringAttribute[] = {
  0x00, 0x01, 0x00, 0x0c,
  0x21, 0x12, 0xa4, 0x42,
  0xe3, 0xa9, 0x46, 0xe1,
  0x7c, 0x00, 0xc2, 0x62,
  0x54, 0x08, 0x01, 0x00,
  0x00, 0x06, 0x00, 0x08,  // username attribute (length 8)
  0x61, 0x62, 0x63, 0x64,  // abcdefgh
  0x65, 0x66, 0x67, 0x68
};

// Message with an unknown but comprehensible optional attribute.
// Parsing should succeed despite this unknown attribute.
static const unsigned char kStunMessageWithUnknownAttribute[] = {
  0x00, 0x01, 0x00, 0x14,
  0x21, 0x12, 0xa4, 0x42,
  0xe3, 0xa9, 0x46, 0xe1,
  0x7c, 0x00, 0xc2, 0x62,
  0x54, 0x08, 0x01, 0x00,
  0x00, 0xaa, 0x00, 0x07,  // Unknown attribute, length 7 (needs padding!)
  0x61, 0x62, 0x63, 0x64,  // abcdefg + padding
  0x65, 0x66, 0x67, 0x00,
  0x00, 0x06, 0x00, 0x03,  // Followed by a known attribute we can
  0x61, 0x62, 0x63, 0x00   // check for (username of length 3)
};

// ByteString Attribute (username) with padding byte
static const unsigned char kStunMessageWithPaddedByteStringAttribute[] = {
  0x00, 0x01, 0x00, 0x08,
  0x21, 0x12, 0xa4, 0x42,
  0xe3, 0xa9, 0x46, 0xe1,
  0x7c, 0x00, 0xc2, 0x62,
  0x54, 0x08, 0x01, 0x00,
  0x00, 0x06, 0x00, 0x03,  // username attribute (length 3)
  0x61, 0x62, 0x63, 0xcc   // abc
};

// Message with an Unknown Attributes (uint16 list) attribute.
static const unsigned char kStunMessageWithUInt16ListAttribute[] = {
  0x00, 0x01, 0x00, 0x0c,
  0x21, 0x12, 0xa4, 0x42,
  0xe3, 0xa9, 0x46, 0xe1,
  0x7c, 0x00, 0xc2, 0x62,
  0x54, 0x08, 0x01, 0x00,
  0x00, 0x0a, 0x00, 0x06,  // username attribute (length 6)
  0x00, 0x01, 0x10, 0x00,  // three attributes plus padding
  0xAB, 0xCU, 0xBE, 0xEF
};

// Error response message (unauthorized)
static const unsigned char kStunMessageWithErrorAttribute[] = {
  0x01, 0x11, 0x00, 0x14,
  0x21, 0x12, 0xa4, 0x42,
  0x29, 0x1f, 0xcd, 0x7c,
  0xba, 0x58, 0xab, 0xd7,
  0xf2, 0x41, 0x01, 0x00,
  0x00, 0x09, 0x00, 0x10,
  0x00, 0x00, 0x04, 0x01,
  0x55, 0x6e, 0x61, 0x75,
  0x74, 0x68, 0x6f, 0x72,
  0x69, 0x7a, 0x65, 0x64
};

// Sample messages with an invalid length Field

// The actual length in bytes of the invalid messages (including STUN header)
static const int kRealLengthOfInvalidLengthTestCases = 32;

static const unsigned char kStunMessageWithZeroLength[] = {
  0x00, 0x01, 0x00, 0x00,  // length of 0 (last 2 bytes)
  0x21, 0x12, 0xA4, 0x42,  // magic cookie
  '0', '1', '2', '3',      // transaction id
  '4', '5', '6', '7',
  '8', '9', 'a', 'b',
  0x00, 0x20, 0x00, 0x08,  // xor mapped address
  0x00, 0x01, 0x21, 0x1F,
  0x21, 0x12, 0xA4, 0x53,
};

static const unsigned char kStunMessageWithExcessLength[] = {
  0x00, 0x01, 0x00, 0x55,  // length of 85
  0x21, 0x12, 0xA4, 0x42,  // magic cookie
  '0', '1', '2', '3',      // transaction id
  '4', '5', '6', '7',
  '8', '9', 'a', 'b',
  0x00, 0x20, 0x00, 0x08,  // xor mapped address
  0x00, 0x01, 0x21, 0x1F,
  0x21, 0x12, 0xA4, 0x53,
};

static const unsigned char kStunMessageWithSmallLength[] = {
  0x00, 0x01, 0x00, 0x03,  // length of 3
  0x21, 0x12, 0xA4, 0x42,  // magic cookie
  '0', '1', '2', '3',      // transaction id
  '4', '5', '6', '7',
  '8', '9', 'a', 'b',
  0x00, 0x20, 0x00, 0x08,  // xor mapped address
  0x00, 0x01, 0x21, 0x1F,
  0x21, 0x12, 0xA4, 0x53,
};

// RTCP packet, for testing we correctly ignore non stun packet types.
// V=2, P=false, RC=0, Type=200, Len=6, Sender-SSRC=85, etc
static const unsigned char kRtcpPacket[] = {
  0x80, 0xc8, 0x00, 0x06, 0x00, 0x00, 0x00, 0x55,
  0xce, 0xa5, 0x18, 0x3a, 0x39, 0xcc, 0x7d, 0x09,
  0x23, 0xed, 0x19, 0x07, 0x00, 0x00, 0x01, 0x56,
  0x00, 0x03, 0x73, 0x50,
};

// RFC5769 Test Vectors
// Software name (request):  "STUN test client" (without quotes)
// Software name (response): "test vector" (without quotes)
// Username:  "evtj:h6vY" (without quotes)
// Password:  "VOkJxbRl1RmTxUk/WvJxBt" (without quotes)
static const unsigned char kRfc5769SampleMsgTransactionId[] = {
  0xb7, 0xe7, 0xa7, 0x01, 0xbc, 0x34, 0xd6, 0x86, 0xfa, 0x87, 0xdf, 0xae
};
static const char kRfc5769SampleMsgClientSoftware[] = "STUN test client";
static const char kRfc5769SampleMsgServerSoftware[] = "test vector";
static const char kRfc5769SampleMsgUsername[] = "evtj:h6vY";
static const char kRfc5769SampleMsgPassword[] = "VOkJxbRl1RmTxUk/WvJxBt";
static const talk_base::SocketAddress kRfc5769SampleMsgMappedAddress(
    "192.0.2.1", 32853);
static const talk_base::SocketAddress kRfc5769SampleMsgIPv6MappedAddress(
    "2001:db8:1234:5678:11:2233:4455:6677", 32853);

static const unsigned char kRfc5769SampleMsgWithAuthTransactionId[] = {
  0x78, 0xad, 0x34, 0x33, 0xc6, 0xad, 0x72, 0xc0, 0x29, 0xda, 0x41, 0x2e
};
static const char kRfc5769SampleMsgWithAuthUsername[] =
    "\xe3\x83\x9e\xe3\x83\x88\xe3\x83\xaa\xe3\x83\x83\xe3\x82\xaf\xe3\x82\xb9";
static const char kRfc5769SampleMsgWithAuthPassword[] = "TheMatrIX";
static const char kRfc5769SampleMsgWithAuthNonce[] =
    "f//499k954d6OL34oL9FSTvy64sA";
static const char kRfc5769SampleMsgWithAuthRealm[] = "example.org";

// 2.1.  Sample Request
static const unsigned char kRfc5769SampleRequest[] = {
  0x00, 0x01, 0x00, 0x58,   //    Request type and message length
  0x21, 0x12, 0xa4, 0x42,   //    Magic cookie
  0xb7, 0xe7, 0xa7, 0x01,   // }
  0xbc, 0x34, 0xd6, 0x86,   // }  Transaction ID
  0xfa, 0x87, 0xdf, 0xae,   // }
  0x80, 0x22, 0x00, 0x10,   //    SOFTWARE attribute header
  0x53, 0x54, 0x55, 0x4e,   // }
  0x20, 0x74, 0x65, 0x73,   // }  User-agent...
  0x74, 0x20, 0x63, 0x6c,   // }  ...name
  0x69, 0x65, 0x6e, 0x74,   // }
  0x00, 0x24, 0x00, 0x04,   //    PRIORITY attribute header
  0x6e, 0x00, 0x01, 0xff,   //    ICE priority value
  0x80, 0x29, 0x00, 0x08,   //    ICE-CONTROLLED attribute header
  0x93, 0x2f, 0xf9, 0xb1,   // }  Pseudo-random tie breaker...
  0x51, 0x26, 0x3b, 0x36,   // }   ...for ICE control
  0x00, 0x06, 0x00, 0x09,   //    USERNAME attribute header
  0x65, 0x76, 0x74, 0x6a,   // }
  0x3a, 0x68, 0x36, 0x76,   // }  Username (9 bytes) and padding (3 bytes)
  0x59, 0x20, 0x20, 0x20,   // }
  0x00, 0x08, 0x00, 0x14,   //    MESSAGE-INTEGRITY attribute header
  0x9a, 0xea, 0xa7, 0x0c,   // }
  0xbf, 0xd8, 0xcb, 0x56,   // }
  0x78, 0x1e, 0xf2, 0xb5,   // }  HMAC-SHA1 fingerprint
  0xb2, 0xd3, 0xf2, 0x49,   // }
  0xc1, 0xb5, 0x71, 0xa2,   // }
  0x80, 0x28, 0x00, 0x04,   //    FINGERPRINT attribute header
  0xe5, 0x7a, 0x3b, 0xcf    //    CRC32 fingerprint
};

// 2.2.  Sample IPv4 Response
static const unsigned char kRfc5769SampleResponse[] = {
  0x01, 0x01, 0x00, 0x3c,  //     Response type and message length
  0x21, 0x12, 0xa4, 0x42,  //     Magic cookie
  0xb7, 0xe7, 0xa7, 0x01,  // }
  0xbc, 0x34, 0xd6, 0x86,  // }  Transaction ID
  0xfa, 0x87, 0xdf, 0xae,  // }
  0x80, 0x22, 0x00, 0x0b,  //    SOFTWARE attribute header
  0x74, 0x65, 0x73, 0x74,  // }
  0x20, 0x76, 0x65, 0x63,  // }  UTF-8 server name
  0x74, 0x6f, 0x72, 0x20,  // }
  0x00, 0x20, 0x00, 0x08,  //    XOR-MAPPED-ADDRESS attribute header
  0x00, 0x01, 0xa1, 0x47,  //    Address family (IPv4) and xor'd mapped port
  0xe1, 0x12, 0xa6, 0x43,  //    Xor'd mapped IPv4 address
  0x00, 0x08, 0x00, 0x14,  //    MESSAGE-INTEGRITY attribute header
  0x2b, 0x91, 0xf5, 0x99,  // }
  0xfd, 0x9e, 0x90, 0xc3,  // }
  0x8c, 0x74, 0x89, 0xf9,  // }  HMAC-SHA1 fingerprint
  0x2a, 0xf9, 0xba, 0x53,  // }
  0xf0, 0x6b, 0xe7, 0xd7,  // }
  0x80, 0x28, 0x00, 0x04,  //    FINGERPRINT attribute header
  0xc0, 0x7d, 0x4c, 0x96   //    CRC32 fingerprint
};

// 2.3.  Sample IPv6 Response
static const unsigned char kRfc5769SampleResponseIPv6[] = {
  0x01, 0x01, 0x00, 0x48,  //    Response type and message length
  0x21, 0x12, 0xa4, 0x42,  //    Magic cookie
  0xb7, 0xe7, 0xa7, 0x01,  // }
  0xbc, 0x34, 0xd6, 0x86,  // }  Transaction ID
  0xfa, 0x87, 0xdf, 0xae,  // }
  0x80, 0x22, 0x00, 0x0b,  //    SOFTWARE attribute header
  0x74, 0x65, 0x73, 0x74,  // }
  0x20, 0x76, 0x65, 0x63,  // }  UTF-8 server name
  0x74, 0x6f, 0x72, 0x20,  // }
  0x00, 0x20, 0x00, 0x14,  //    XOR-MAPPED-ADDRESS attribute header
  0x00, 0x02, 0xa1, 0x47,  //    Address family (IPv6) and xor'd mapped port.
  0x01, 0x13, 0xa9, 0xfa,  // }
  0xa5, 0xd3, 0xf1, 0x79,  // }  Xor'd mapped IPv6 address
  0xbc, 0x25, 0xf4, 0xb5,  // }
  0xbe, 0xd2, 0xb9, 0xd9,  // }
  0x00, 0x08, 0x00, 0x14,  //    MESSAGE-INTEGRITY attribute header
  0xa3, 0x82, 0x95, 0x4e,  // }
  0x4b, 0xe6, 0x7b, 0xf1,  // }
  0x17, 0x84, 0xc9, 0x7c,  // }  HMAC-SHA1 fingerprint
  0x82, 0x92, 0xc2, 0x75,  // }
  0xbf, 0xe3, 0xed, 0x41,  // }
  0x80, 0x28, 0x00, 0x04,  //    FINGERPRINT attribute header
  0xc8, 0xfb, 0x0b, 0x4c   //    CRC32 fingerprint
};

// 2.4.  Sample Request with Long-Term Authentication
static const unsigned char kRfc5769SampleRequestLongTermAuth[] = {
  0x00, 0x01, 0x00, 0x60,  //    Request type and message length
  0x21, 0x12, 0xa4, 0x42,  //    Magic cookie
  0x78, 0xad, 0x34, 0x33,  // }
  0xc6, 0xad, 0x72, 0xc0,  // }  Transaction ID
  0x29, 0xda, 0x41, 0x2e,  // }
  0x00, 0x06, 0x00, 0x12,  //    USERNAME attribute header
  0xe3, 0x83, 0x9e, 0xe3,  // }
  0x83, 0x88, 0xe3, 0x83,  // }
  0xaa, 0xe3, 0x83, 0x83,  // }  Username value (18 bytes) and padding (2 bytes)
  0xe3, 0x82, 0xaf, 0xe3,  // }
  0x82, 0xb9, 0x00, 0x00,  // }
  0x00, 0x15, 0x00, 0x1c,  //    NONCE attribute header
  0x66, 0x2f, 0x2f, 0x34,  // }
  0x39, 0x39, 0x6b, 0x39,  // }
  0x35, 0x34, 0x64, 0x36,  // }
  0x4f, 0x4c, 0x33, 0x34,  // }  Nonce value
  0x6f, 0x4c, 0x39, 0x46,  // }
  0x53, 0x54, 0x76, 0x79,  // }
  0x36, 0x34, 0x73, 0x41,  // }
  0x00, 0x14, 0x00, 0x0b,  //    REALM attribute header
  0x65, 0x78, 0x61, 0x6d,  // }
  0x70, 0x6c, 0x65, 0x2e,  // }  Realm value (11 bytes) and padding (1 byte)
  0x6f, 0x72, 0x67, 0x00,  // }
  0x00, 0x08, 0x00, 0x14,  //    MESSAGE-INTEGRITY attribute header
  0xf6, 0x70, 0x24, 0x65,  // }
  0x6d, 0xd6, 0x4a, 0x3e,  // }
  0x02, 0xb8, 0xe0, 0x71,  // }  HMAC-SHA1 fingerprint
  0x2e, 0x85, 0xc9, 0xa2,  // }
  0x8c, 0xa8, 0x96, 0x66   // }
};

// Length parameter is changed to 0x38 from 0x58.
// AddMessageIntegrity will add MI information and update the length param
// accordingly.
static const unsigned char kRfc5769SampleRequestWithoutMI[] = {
  0x00, 0x01, 0x00, 0x38,  //    Request type and message length
  0x21, 0x12, 0xa4, 0x42,  //    Magic cookie
  0xb7, 0xe7, 0xa7, 0x01,  // }
  0xbc, 0x34, 0xd6, 0x86,  // }  Transaction ID
  0xfa, 0x87, 0xdf, 0xae,  // }
  0x80, 0x22, 0x00, 0x10,  //    SOFTWARE attribute header
  0x53, 0x54, 0x55, 0x4e,  // }
  0x20, 0x74, 0x65, 0x73,  // }  User-agent...
  0x74, 0x20, 0x63, 0x6c,  // }  ...name
  0x69, 0x65, 0x6e, 0x74,  // }
  0x00, 0x24, 0x00, 0x04,  //    PRIORITY attribute header
  0x6e, 0x00, 0x01, 0xff,  //    ICE priority value
  0x80, 0x29, 0x00, 0x08,  //    ICE-CONTROLLED attribute header
  0x93, 0x2f, 0xf9, 0xb1,  // }  Pseudo-random tie breaker...
  0x51, 0x26, 0x3b, 0x36,  // }   ...for ICE control
  0x00, 0x06, 0x00, 0x09,  //    USERNAME attribute header
  0x65, 0x76, 0x74, 0x6a,  // }
  0x3a, 0x68, 0x36, 0x76,  // }  Username (9 bytes) and padding (3 bytes)
  0x59, 0x20, 0x20, 0x20   // }
};

// This HMAC differs from the RFC 5769 SampleRequest message. This differs
// because spec uses 0x20 for the padding where as our implementation uses 0.
static const unsigned char kCalculatedHmac1[] = {
  0x79, 0x07, 0xc2, 0xd2,  // }
  0xed, 0xbf, 0xea, 0x48,  // }
  0x0e, 0x4c, 0x76, 0xd8,  // }  HMAC-SHA1 fingerprint
  0x29, 0x62, 0xd5, 0xc3,  // }
  0x74, 0x2a, 0xf9, 0xe3   // }
};

// Length parameter is changed to 0x1c from 0x3c.
// AddMessageIntegrity will add MI information and update the length param
// accordingly.
static const unsigned char kRfc5769SampleResponseWithoutMI[] = {
  0x01, 0x01, 0x00, 0x1c,  //    Response type and message length
  0x21, 0x12, 0xa4, 0x42,  //    Magic cookie
  0xb7, 0xe7, 0xa7, 0x01,  // }
  0xbc, 0x34, 0xd6, 0x86,  // }  Transaction ID
  0xfa, 0x87, 0xdf, 0xae,  // }
  0x80, 0x22, 0x00, 0x0b,  //    SOFTWARE attribute header
  0x74, 0x65, 0x73, 0x74,  // }
  0x20, 0x76, 0x65, 0x63,  // }  UTF-8 server name
  0x74, 0x6f, 0x72, 0x20,  // }
  0x00, 0x20, 0x00, 0x08,  //    XOR-MAPPED-ADDRESS attribute header
  0x00, 0x01, 0xa1, 0x47,  //    Address family (IPv4) and xor'd mapped port
  0xe1, 0x12, 0xa6, 0x43   //    Xor'd mapped IPv4 address
};

// This HMAC differs from the RFC 5769 SampleResponse message. This differs
// because spec uses 0x20 for the padding where as our implementation uses 0.
static const unsigned char kCalculatedHmac2[] = {
  0x5d, 0x6b, 0x58, 0xbe,  // }
  0xad, 0x94, 0xe0, 0x7e,  // }
  0xef, 0x0d, 0xfc, 0x12,  // }  HMAC-SHA1 fingerprint
  0x82, 0xa2, 0xbd, 0x08,  // }
  0x43, 0x14, 0x10, 0x28   // }
};

// A transaction ID without the 'magic cookie' portion
// pjnat's test programs use this transaction ID a lot.
const unsigned char kTestTransactionId1[] = { 0x029, 0x01f, 0x0cd, 0x07c,
                                              0x0ba, 0x058, 0x0ab, 0x0d7,
                                              0x0f2, 0x041, 0x001, 0x000 };

// They use this one sometimes too.
const unsigned char kTestTransactionId2[] = { 0x0e3, 0x0a9, 0x046, 0x0e1,
                                              0x07c, 0x000, 0x0c2, 0x062,
                                              0x054, 0x008, 0x001, 0x000 };

const in6_addr kIPv6TestAddress1 = { { { 0x24, 0x01, 0xfa, 0x00,
                                         0x00, 0x04, 0x10, 0x00,
                                         0xbe, 0x30, 0x5b, 0xff,
                                         0xfe, 0xe5, 0x00, 0xc3 } } };
const in6_addr kIPv6TestAddress2 = { { { 0x24, 0x01, 0xfa, 0x00,
                                         0x00, 0x04, 0x10, 0x12,
                                         0x06, 0x0c, 0xce, 0xff,
                                         0xfe, 0x1f, 0x61, 0xa4 } } };

#ifdef POSIX
const in_addr kIPv4TestAddress1 =  { 0xe64417ac };
#elif defined WIN32
// Windows in_addr has a union with a uchar[] array first.
const in_addr kIPv4TestAddress1 =  { { 0x0ac, 0x017, 0x044, 0x0e6 } };
#endif
const char kTestUserName1[] = "abcdefgh";
const char kTestUserName2[] = "abc";
const char kTestErrorReason[] = "Unauthorized";
const int kTestErrorClass = 4;
const int kTestErrorNumber = 1;
const int kTestErrorCode = 401;

const int kTestMessagePort1 = 59977;
const int kTestMessagePort2 = 47233;
const int kTestMessagePort3 = 56743;
const int kTestMessagePort4 = 40444;

#define ReadStunMessage(X, Y) ReadStunMessageTestCase(X, Y, sizeof(Y));

// Test that the GetStun*Type and IsStun*Type methods work as expected.
TEST_F(StunTest, MessageTypes) {
  EXPECT_EQ(STUN_BINDING_RESPONSE,
      GetStunSuccessResponseType(STUN_BINDING_REQUEST));
  EXPECT_EQ(STUN_BINDING_ERROR_RESPONSE,
      GetStunErrorResponseType(STUN_BINDING_REQUEST));
  EXPECT_EQ(-1, GetStunSuccessResponseType(STUN_BINDING_INDICATION));
  EXPECT_EQ(-1, GetStunSuccessResponseType(STUN_BINDING_RESPONSE));
  EXPECT_EQ(-1, GetStunSuccessResponseType(STUN_BINDING_ERROR_RESPONSE));
  EXPECT_EQ(-1, GetStunErrorResponseType(STUN_BINDING_INDICATION));
  EXPECT_EQ(-1, GetStunErrorResponseType(STUN_BINDING_RESPONSE));
  EXPECT_EQ(-1, GetStunErrorResponseType(STUN_BINDING_ERROR_RESPONSE));

  int types[] = {
    STUN_BINDING_REQUEST, STUN_BINDING_INDICATION,
    STUN_BINDING_RESPONSE, STUN_BINDING_ERROR_RESPONSE
  };
  for (int i = 0; i < ARRAY_SIZE(types); ++i) {
    EXPECT_EQ(i == 0, IsStunRequestType(types[i]));
    EXPECT_EQ(i == 1, IsStunIndicationType(types[i]));
    EXPECT_EQ(i == 2, IsStunSuccessResponseType(types[i]));
    EXPECT_EQ(i == 3, IsStunErrorResponseType(types[i]));
    EXPECT_EQ(1, types[i] & 0xFEEF);
  }
}

TEST_F(StunTest, ReadMessageWithIPv4AddressAttribute) {
  StunMessage msg;
  size_t size = ReadStunMessage(&msg, kStunMessageWithIPv4MappedAddress);
  CheckStunHeader(msg, STUN_BINDING_RESPONSE, size);
  CheckStunTransactionID(msg, kTestTransactionId1, kStunTransactionIdLength);

  const StunAddressAttribute* addr = msg.GetAddress(STUN_ATTR_MAPPED_ADDRESS);
  talk_base::IPAddress test_address(kIPv4TestAddress1);
  CheckStunAddressAttribute(addr, STUN_ADDRESS_IPV4,
                            kTestMessagePort4, test_address);
}

TEST_F(StunTest, ReadMessageWithIPv4XorAddressAttribute) {
  StunMessage msg;
  StunMessage msg2;
  size_t size = ReadStunMessage(&msg, kStunMessageWithIPv4XorMappedAddress);
  CheckStunHeader(msg, STUN_BINDING_RESPONSE, size);
  CheckStunTransactionID(msg, kTestTransactionId1, kStunTransactionIdLength);

  const StunAddressAttribute* addr =
      msg.GetAddress(STUN_ATTR_XOR_MAPPED_ADDRESS);
  talk_base::IPAddress test_address(kIPv4TestAddress1);
  CheckStunAddressAttribute(addr, STUN_ADDRESS_IPV4,
                            kTestMessagePort3, test_address);
}

TEST_F(StunTest, ReadMessageWithIPv6AddressAttribute) {
  StunMessage msg;
  size_t size = ReadStunMessage(&msg, kStunMessageWithIPv6MappedAddress);
  CheckStunHeader(msg, STUN_BINDING_REQUEST, size);
  CheckStunTransactionID(msg, kTestTransactionId1, kStunTransactionIdLength);

  talk_base::IPAddress test_address(kIPv6TestAddress1);

  const StunAddressAttribute* addr = msg.GetAddress(STUN_ATTR_MAPPED_ADDRESS);
  CheckStunAddressAttribute(addr, STUN_ADDRESS_IPV6,
                            kTestMessagePort2, test_address);
}

TEST_F(StunTest, ReadMessageWithInvalidAddressAttribute) {
  StunMessage msg;
  size_t size = ReadStunMessage(&msg, kStunMessageWithIPv6MappedAddress);
  CheckStunHeader(msg, STUN_BINDING_REQUEST, size);
  CheckStunTransactionID(msg, kTestTransactionId1, kStunTransactionIdLength);

  talk_base::IPAddress test_address(kIPv6TestAddress1);

  const StunAddressAttribute* addr = msg.GetAddress(STUN_ATTR_MAPPED_ADDRESS);
  CheckStunAddressAttribute(addr, STUN_ADDRESS_IPV6,
                            kTestMessagePort2, test_address);
}

TEST_F(StunTest, ReadMessageWithIPv6XorAddressAttribute) {
  StunMessage msg;
  size_t size = ReadStunMessage(&msg, kStunMessageWithIPv6XorMappedAddress);

  talk_base::IPAddress test_address(kIPv6TestAddress1);

  CheckStunHeader(msg, STUN_BINDING_RESPONSE, size);
  CheckStunTransactionID(msg, kTestTransactionId2, kStunTransactionIdLength);

  const StunAddressAttribute* addr =
      msg.GetAddress(STUN_ATTR_XOR_MAPPED_ADDRESS);
  CheckStunAddressAttribute(addr, STUN_ADDRESS_IPV6,
                            kTestMessagePort1, test_address);
}

// Read the RFC5389 fields from the RFC5769 sample STUN request.
TEST_F(StunTest, ReadRfc5769RequestMessage) {
  StunMessage msg;
  size_t size = ReadStunMessage(&msg, kRfc5769SampleRequest);
  CheckStunHeader(msg, STUN_BINDING_REQUEST, size);
  CheckStunTransactionID(msg, kRfc5769SampleMsgTransactionId,
                         kStunTransactionIdLength);

  const StunByteStringAttribute* software =
      msg.GetByteString(STUN_ATTR_SOFTWARE);
  ASSERT_TRUE(software != NULL);
  EXPECT_EQ(kRfc5769SampleMsgClientSoftware, software->GetString());

  const StunByteStringAttribute* username =
      msg.GetByteString(STUN_ATTR_USERNAME);
  ASSERT_TRUE(username != NULL);
  EXPECT_EQ(kRfc5769SampleMsgUsername, username->GetString());

  // Actual M-I value checked in a later test.
  ASSERT_TRUE(msg.GetByteString(STUN_ATTR_MESSAGE_INTEGRITY) != NULL);

  // Fingerprint checked in a later test, but double-check the value here.
  const StunUInt32Attribute* fingerprint =
      msg.GetUInt32(STUN_ATTR_FINGERPRINT);
  ASSERT_TRUE(fingerprint != NULL);
  EXPECT_EQ(0xe57a3bcf, fingerprint->value());
}

// Read the RFC5389 fields from the RFC5769 sample STUN response.
TEST_F(StunTest, ReadRfc5769ResponseMessage) {
  StunMessage msg;
  size_t size = ReadStunMessage(&msg, kRfc5769SampleResponse);
  CheckStunHeader(msg, STUN_BINDING_RESPONSE, size);
  CheckStunTransactionID(msg, kRfc5769SampleMsgTransactionId,
                         kStunTransactionIdLength);

  const StunByteStringAttribute* software =
      msg.GetByteString(STUN_ATTR_SOFTWARE);
  ASSERT_TRUE(software != NULL);
  EXPECT_EQ(kRfc5769SampleMsgServerSoftware, software->GetString());

  const StunAddressAttribute* mapped_address =
      msg.GetAddress(STUN_ATTR_XOR_MAPPED_ADDRESS);
  ASSERT_TRUE(mapped_address != NULL);
  EXPECT_EQ(kRfc5769SampleMsgMappedAddress, mapped_address->GetAddress());

  // Actual M-I and fingerprint checked in later tests.
  ASSERT_TRUE(msg.GetByteString(STUN_ATTR_MESSAGE_INTEGRITY) != NULL);
  ASSERT_TRUE(msg.GetUInt32(STUN_ATTR_FINGERPRINT) != NULL);
}

// Read the RFC5389 fields from the RFC5769 sample STUN response for IPv6.
TEST_F(StunTest, ReadRfc5769ResponseMessageIPv6) {
  StunMessage msg;
  size_t size = ReadStunMessage(&msg, kRfc5769SampleResponseIPv6);
  CheckStunHeader(msg, STUN_BINDING_RESPONSE, size);
  CheckStunTransactionID(msg, kRfc5769SampleMsgTransactionId,
                         kStunTransactionIdLength);

  const StunByteStringAttribute* software =
      msg.GetByteString(STUN_ATTR_SOFTWARE);
  ASSERT_TRUE(software != NULL);
  EXPECT_EQ(kRfc5769SampleMsgServerSoftware, software->GetString());

  const StunAddressAttribute* mapped_address =
      msg.GetAddress(STUN_ATTR_XOR_MAPPED_ADDRESS);
  ASSERT_TRUE(mapped_address != NULL);
  EXPECT_EQ(kRfc5769SampleMsgIPv6MappedAddress, mapped_address->GetAddress());

  // Actual M-I and fingerprint checked in later tests.
  ASSERT_TRUE(msg.GetByteString(STUN_ATTR_MESSAGE_INTEGRITY) != NULL);
  ASSERT_TRUE(msg.GetUInt32(STUN_ATTR_FINGERPRINT) != NULL);
}

// Read the RFC5389 fields from the RFC5769 sample STUN response with auth.
TEST_F(StunTest, ReadRfc5769RequestMessageLongTermAuth) {
  StunMessage msg;
  size_t size = ReadStunMessage(&msg, kRfc5769SampleRequestLongTermAuth);
  CheckStunHeader(msg, STUN_BINDING_REQUEST, size);
  CheckStunTransactionID(msg, kRfc5769SampleMsgWithAuthTransactionId,
                         kStunTransactionIdLength);

  const StunByteStringAttribute* username =
      msg.GetByteString(STUN_ATTR_USERNAME);
  ASSERT_TRUE(username != NULL);
  EXPECT_EQ(kRfc5769SampleMsgWithAuthUsername, username->GetString());

  const StunByteStringAttribute* nonce =
      msg.GetByteString(STUN_ATTR_NONCE);
  ASSERT_TRUE(nonce != NULL);
  EXPECT_EQ(kRfc5769SampleMsgWithAuthNonce, nonce->GetString());

  const StunByteStringAttribute* realm =
      msg.GetByteString(STUN_ATTR_REALM);
  ASSERT_TRUE(realm != NULL);
  EXPECT_EQ(kRfc5769SampleMsgWithAuthRealm, realm->GetString());

  // No fingerprint, actual M-I checked in later tests.
  ASSERT_TRUE(msg.GetByteString(STUN_ATTR_MESSAGE_INTEGRITY) != NULL);
  ASSERT_TRUE(msg.GetUInt32(STUN_ATTR_FINGERPRINT) == NULL);
}

// The RFC3489 packet in this test is the same as
// kStunMessageWithIPv4MappedAddress, but with a different value where the
// magic cookie was.
TEST_F(StunTest, ReadLegacyMessage) {
  unsigned char rfc3489_packet[sizeof(kStunMessageWithIPv4MappedAddress)];
  memcpy(rfc3489_packet, kStunMessageWithIPv4MappedAddress,
      sizeof(kStunMessageWithIPv4MappedAddress));
  // Overwrite the magic cookie here.
  memcpy(&rfc3489_packet[4], "ABCD", 4);

  StunMessage msg;
  size_t size = ReadStunMessage(&msg, rfc3489_packet);
  CheckStunHeader(msg, STUN_BINDING_RESPONSE, size);
  CheckStunTransactionID(msg, &rfc3489_packet[4], kStunTransactionIdLength + 4);

  const StunAddressAttribute* addr = msg.GetAddress(STUN_ATTR_MAPPED_ADDRESS);
  talk_base::IPAddress test_address(kIPv4TestAddress1);
  CheckStunAddressAttribute(addr, STUN_ADDRESS_IPV4,
                            kTestMessagePort4, test_address);
}

TEST_F(StunTest, SetIPv6XorAddressAttributeOwner) {
  StunMessage msg;
  StunMessage msg2;
  size_t size = ReadStunMessage(&msg, kStunMessageWithIPv6XorMappedAddress);

  talk_base::IPAddress test_address(kIPv6TestAddress1);

  CheckStunHeader(msg, STUN_BINDING_RESPONSE, size);
  CheckStunTransactionID(msg, kTestTransactionId2, kStunTransactionIdLength);

  const StunAddressAttribute* addr =
      msg.GetAddress(STUN_ATTR_XOR_MAPPED_ADDRESS);
  CheckStunAddressAttribute(addr, STUN_ADDRESS_IPV6,
                            kTestMessagePort1, test_address);

  // Owner with a different transaction ID.
  msg2.SetTransactionID("ABCDABCDABCD");
  StunXorAddressAttribute addr2(STUN_ATTR_XOR_MAPPED_ADDRESS, 20, NULL);
  addr2.SetIP(addr->ipaddr());
  addr2.SetPort(addr->port());
  addr2.SetOwner(&msg2);
  // The internal IP address shouldn't change.
  ASSERT_EQ(addr2.ipaddr(), addr->ipaddr());

  talk_base::ByteBuffer correct_buf;
  talk_base::ByteBuffer wrong_buf;
  EXPECT_TRUE(addr->Write(&correct_buf));
  EXPECT_TRUE(addr2.Write(&wrong_buf));
  // But when written out, the buffers should look different.
  ASSERT_NE(0,
            memcmp(correct_buf.Data(), wrong_buf.Data(), wrong_buf.Length()));
  // And when reading a known good value, the address should be wrong.
  addr2.Read(&correct_buf);
  ASSERT_NE(addr->ipaddr(), addr2.ipaddr());
  addr2.SetIP(addr->ipaddr());
  addr2.SetPort(addr->port());
  // Try writing with no owner at all, should fail and write nothing.
  addr2.SetOwner(NULL);
  ASSERT_EQ(addr2.ipaddr(), addr->ipaddr());
  wrong_buf.Consume(wrong_buf.Length());
  EXPECT_FALSE(addr2.Write(&wrong_buf));
  ASSERT_EQ(0U, wrong_buf.Length());
}

TEST_F(StunTest, SetIPv4XorAddressAttributeOwner) {
  // Unlike the IPv6XorAddressAttributeOwner test, IPv4 XOR address attributes
  // should _not_ be affected by a change in owner. IPv4 XOR address uses the
  // magic cookie value which is fixed.
  StunMessage msg;
  StunMessage msg2;
  size_t size = ReadStunMessage(&msg, kStunMessageWithIPv4XorMappedAddress);

  talk_base::IPAddress test_address(kIPv4TestAddress1);

  CheckStunHeader(msg, STUN_BINDING_RESPONSE, size);
  CheckStunTransactionID(msg, kTestTransactionId1, kStunTransactionIdLength);

  const StunAddressAttribute* addr =
      msg.GetAddress(STUN_ATTR_XOR_MAPPED_ADDRESS);
  CheckStunAddressAttribute(addr, STUN_ADDRESS_IPV4,
                            kTestMessagePort3, test_address);

  // Owner with a different transaction ID.
  msg2.SetTransactionID("ABCDABCDABCD");
  StunXorAddressAttribute addr2(STUN_ATTR_XOR_MAPPED_ADDRESS, 20, NULL);
  addr2.SetIP(addr->ipaddr());
  addr2.SetPort(addr->port());
  addr2.SetOwner(&msg2);
  // The internal IP address shouldn't change.
  ASSERT_EQ(addr2.ipaddr(), addr->ipaddr());

  talk_base::ByteBuffer correct_buf;
  talk_base::ByteBuffer wrong_buf;
  EXPECT_TRUE(addr->Write(&correct_buf));
  EXPECT_TRUE(addr2.Write(&wrong_buf));
  // The same address data should be written.
  ASSERT_EQ(0,
            memcmp(correct_buf.Data(), wrong_buf.Data(), wrong_buf.Length()));
  // And an attribute should be able to un-XOR an address belonging to a message
  // with a different transaction ID.
  EXPECT_TRUE(addr2.Read(&correct_buf));
  ASSERT_EQ(addr->ipaddr(), addr2.ipaddr());

  // However, no owner is still an error, should fail and write nothing.
  addr2.SetOwner(NULL);
  ASSERT_EQ(addr2.ipaddr(), addr->ipaddr());
  wrong_buf.Consume(wrong_buf.Length());
  EXPECT_FALSE(addr2.Write(&wrong_buf));
}

TEST_F(StunTest, CreateIPv6AddressAttribute) {
  talk_base::IPAddress test_ip(kIPv6TestAddress2);

  StunAddressAttribute* addr =
      StunAttribute::CreateAddress(STUN_ATTR_MAPPED_ADDRESS);
  talk_base::SocketAddress test_addr(test_ip, kTestMessagePort2);
  addr->SetAddress(test_addr);

  CheckStunAddressAttribute(addr, STUN_ADDRESS_IPV6,
                            kTestMessagePort2, test_ip);
  delete addr;
}

TEST_F(StunTest, CreateIPv4AddressAttribute) {
  struct in_addr test_in_addr;
  test_in_addr.s_addr = 0xBEB0B0BE;
  talk_base::IPAddress test_ip(test_in_addr);

  StunAddressAttribute* addr =
      StunAttribute::CreateAddress(STUN_ATTR_MAPPED_ADDRESS);
  talk_base::SocketAddress test_addr(test_ip, kTestMessagePort2);
  addr->SetAddress(test_addr);

  CheckStunAddressAttribute(addr, STUN_ADDRESS_IPV4,
                            kTestMessagePort2, test_ip);
  delete addr;
}

// Test that we don't care what order we set the parts of an address
TEST_F(StunTest, CreateAddressInArbitraryOrder) {
  StunAddressAttribute* addr =
  StunAttribute::CreateAddress(STUN_ATTR_DESTINATION_ADDRESS);
  // Port first
  addr->SetPort(kTestMessagePort1);
  addr->SetIP(talk_base::IPAddress(kIPv4TestAddress1));
  ASSERT_EQ(kTestMessagePort1, addr->port());
  ASSERT_EQ(talk_base::IPAddress(kIPv4TestAddress1), addr->ipaddr());

  StunAddressAttribute* addr2 =
  StunAttribute::CreateAddress(STUN_ATTR_DESTINATION_ADDRESS);
  // IP first
  addr2->SetIP(talk_base::IPAddress(kIPv4TestAddress1));
  addr2->SetPort(kTestMessagePort2);
  ASSERT_EQ(kTestMessagePort2, addr2->port());
  ASSERT_EQ(talk_base::IPAddress(kIPv4TestAddress1), addr2->ipaddr());

  delete addr;
  delete addr2;
}

TEST_F(StunTest, WriteMessageWithIPv6AddressAttribute) {
  StunMessage msg;
  size_t size = sizeof(kStunMessageWithIPv6MappedAddress);

  talk_base::IPAddress test_ip(kIPv6TestAddress1);

  msg.SetType(STUN_BINDING_REQUEST);
  msg.SetTransactionID(
      std::string(reinterpret_cast<const char*>(kTestTransactionId1),
                  kStunTransactionIdLength));
  CheckStunTransactionID(msg, kTestTransactionId1, kStunTransactionIdLength);

  StunAddressAttribute* addr =
      StunAttribute::CreateAddress(STUN_ATTR_MAPPED_ADDRESS);
  talk_base::SocketAddress test_addr(test_ip, kTestMessagePort2);
  addr->SetAddress(test_addr);
  EXPECT_TRUE(msg.AddAttribute(addr));

  CheckStunHeader(msg, STUN_BINDING_REQUEST, (size - 20));

  talk_base::ByteBuffer out;
  EXPECT_TRUE(msg.Write(&out));
  ASSERT_EQ(out.Length(), sizeof(kStunMessageWithIPv6MappedAddress));
  int len1 = static_cast<int>(out.Length());
  std::string bytes;
  out.ReadString(&bytes, len1);
  ASSERT_EQ(0, memcmp(bytes.c_str(), kStunMessageWithIPv6MappedAddress, len1));
}

TEST_F(StunTest, WriteMessageWithIPv4AddressAttribute) {
  StunMessage msg;
  size_t size = sizeof(kStunMessageWithIPv4MappedAddress);

  talk_base::IPAddress test_ip(kIPv4TestAddress1);

  msg.SetType(STUN_BINDING_RESPONSE);
  msg.SetTransactionID(
      std::string(reinterpret_cast<const char*>(kTestTransactionId1),
                  kStunTransactionIdLength));
  CheckStunTransactionID(msg, kTestTransactionId1, kStunTransactionIdLength);

  StunAddressAttribute* addr =
      StunAttribute::CreateAddress(STUN_ATTR_MAPPED_ADDRESS);
  talk_base::SocketAddress test_addr(test_ip, kTestMessagePort4);
  addr->SetAddress(test_addr);
  EXPECT_TRUE(msg.AddAttribute(addr));

  CheckStunHeader(msg, STUN_BINDING_RESPONSE, (size - 20));

  talk_base::ByteBuffer out;
  EXPECT_TRUE(msg.Write(&out));
  ASSERT_EQ(out.Length(), sizeof(kStunMessageWithIPv4MappedAddress));
  int len1 = static_cast<int>(out.Length());
  std::string bytes;
  out.ReadString(&bytes, len1);
  ASSERT_EQ(0, memcmp(bytes.c_str(), kStunMessageWithIPv4MappedAddress, len1));
}

TEST_F(StunTest, WriteMessageWithIPv6XorAddressAttribute) {
  StunMessage msg;
  size_t size = sizeof(kStunMessageWithIPv6XorMappedAddress);

  talk_base::IPAddress test_ip(kIPv6TestAddress1);

  msg.SetType(STUN_BINDING_RESPONSE);
  msg.SetTransactionID(
      std::string(reinterpret_cast<const char*>(kTestTransactionId2),
                  kStunTransactionIdLength));
  CheckStunTransactionID(msg, kTestTransactionId2, kStunTransactionIdLength);

  StunAddressAttribute* addr =
      StunAttribute::CreateXorAddress(STUN_ATTR_XOR_MAPPED_ADDRESS);
  talk_base::SocketAddress test_addr(test_ip, kTestMessagePort1);
  addr->SetAddress(test_addr);
  EXPECT_TRUE(msg.AddAttribute(addr));

  CheckStunHeader(msg, STUN_BINDING_RESPONSE, (size - 20));

  talk_base::ByteBuffer out;
  EXPECT_TRUE(msg.Write(&out));
  ASSERT_EQ(out.Length(), sizeof(kStunMessageWithIPv6XorMappedAddress));
  int len1 = static_cast<int>(out.Length());
  std::string bytes;
  out.ReadString(&bytes, len1);
  ASSERT_EQ(0,
            memcmp(bytes.c_str(), kStunMessageWithIPv6XorMappedAddress, len1));
}

TEST_F(StunTest, WriteMessageWithIPv4XoreAddressAttribute) {
  StunMessage msg;
  size_t size = sizeof(kStunMessageWithIPv4XorMappedAddress);

  talk_base::IPAddress test_ip(kIPv4TestAddress1);

  msg.SetType(STUN_BINDING_RESPONSE);
  msg.SetTransactionID(
      std::string(reinterpret_cast<const char*>(kTestTransactionId1),
                  kStunTransactionIdLength));
  CheckStunTransactionID(msg, kTestTransactionId1, kStunTransactionIdLength);

  StunAddressAttribute* addr =
      StunAttribute::CreateXorAddress(STUN_ATTR_XOR_MAPPED_ADDRESS);
  talk_base::SocketAddress test_addr(test_ip, kTestMessagePort3);
  addr->SetAddress(test_addr);
  EXPECT_TRUE(msg.AddAttribute(addr));

  CheckStunHeader(msg, STUN_BINDING_RESPONSE, (size - 20));

  talk_base::ByteBuffer out;
  EXPECT_TRUE(msg.Write(&out));
  ASSERT_EQ(out.Length(), sizeof(kStunMessageWithIPv4XorMappedAddress));
  int len1 = static_cast<int>(out.Length());
  std::string bytes;
  out.ReadString(&bytes, len1);
  ASSERT_EQ(0,
            memcmp(bytes.c_str(), kStunMessageWithIPv4XorMappedAddress, len1));
}

TEST_F(StunTest, ReadByteStringAttribute) {
  StunMessage msg;
  size_t size = ReadStunMessage(&msg, kStunMessageWithByteStringAttribute);

  CheckStunHeader(msg, STUN_BINDING_REQUEST, size);
  CheckStunTransactionID(msg, kTestTransactionId2, kStunTransactionIdLength);
  const StunByteStringAttribute* username =
      msg.GetByteString(STUN_ATTR_USERNAME);
  ASSERT_TRUE(username != NULL);
  EXPECT_EQ(kTestUserName1, username->GetString());
}

TEST_F(StunTest, ReadPaddedByteStringAttribute) {
  StunMessage msg;
  size_t size = ReadStunMessage(&msg,
                                kStunMessageWithPaddedByteStringAttribute);
  ASSERT_NE(0U, size);
  CheckStunHeader(msg, STUN_BINDING_REQUEST, size);
  CheckStunTransactionID(msg, kTestTransactionId2, kStunTransactionIdLength);
  const StunByteStringAttribute* username =
      msg.GetByteString(STUN_ATTR_USERNAME);
  ASSERT_TRUE(username != NULL);
  EXPECT_EQ(kTestUserName2, username->GetString());
}

TEST_F(StunTest, ReadErrorCodeAttribute) {
  StunMessage msg;
  size_t size = ReadStunMessage(&msg, kStunMessageWithErrorAttribute);

  CheckStunHeader(msg, STUN_BINDING_ERROR_RESPONSE, size);
  CheckStunTransactionID(msg, kTestTransactionId1, kStunTransactionIdLength);
  const StunErrorCodeAttribute* errorcode = msg.GetErrorCode();
  ASSERT_TRUE(errorcode != NULL);
  EXPECT_EQ(kTestErrorClass, errorcode->eclass());
  EXPECT_EQ(kTestErrorNumber, errorcode->number());
  EXPECT_EQ(kTestErrorReason, errorcode->reason());
  EXPECT_EQ(kTestErrorCode, errorcode->code());
}

TEST_F(StunTest, ReadMessageWithAUInt16ListAttribute) {
  StunMessage msg;
  size_t size = ReadStunMessage(&msg, kStunMessageWithUInt16ListAttribute);
  CheckStunHeader(msg, STUN_BINDING_REQUEST, size);
  const StunUInt16ListAttribute* types = msg.GetUnknownAttributes();
  ASSERT_TRUE(types != NULL);
  EXPECT_EQ(3U, types->Size());
  EXPECT_EQ(0x1U, types->GetType(0));
  EXPECT_EQ(0x1000U, types->GetType(1));
  EXPECT_EQ(0xAB0CU, types->GetType(2));
}

TEST_F(StunTest, ReadMessageWithAnUnknownAttribute) {
  StunMessage msg;
  size_t size = ReadStunMessage(&msg, kStunMessageWithUnknownAttribute);
  CheckStunHeader(msg, STUN_BINDING_REQUEST, size);

  // Parsing should have succeeded and there should be a USERNAME attribute
  const StunByteStringAttribute* username =
      msg.GetByteString(STUN_ATTR_USERNAME);
  ASSERT_TRUE(username != NULL);
  EXPECT_EQ(kTestUserName2, username->GetString());
}

TEST_F(StunTest, WriteMessageWithAnErrorCodeAttribute) {
  StunMessage msg;
  size_t size = sizeof(kStunMessageWithErrorAttribute);

  msg.SetType(STUN_BINDING_ERROR_RESPONSE);
  msg.SetTransactionID(
      std::string(reinterpret_cast<const char*>(kTestTransactionId1),
                  kStunTransactionIdLength));
  CheckStunTransactionID(msg, kTestTransactionId1, kStunTransactionIdLength);
  StunErrorCodeAttribute* errorcode = StunAttribute::CreateErrorCode();
  errorcode->SetCode(kTestErrorCode);
  errorcode->SetReason(kTestErrorReason);
  EXPECT_TRUE(msg.AddAttribute(errorcode));
  CheckStunHeader(msg, STUN_BINDING_ERROR_RESPONSE, (size - 20));

  talk_base::ByteBuffer out;
  EXPECT_TRUE(msg.Write(&out));
  ASSERT_EQ(size, out.Length());
  // No padding.
  ASSERT_EQ(0, memcmp(out.Data(), kStunMessageWithErrorAttribute, size));
}

TEST_F(StunTest, WriteMessageWithAUInt16ListAttribute) {
  StunMessage msg;
  size_t size = sizeof(kStunMessageWithUInt16ListAttribute);

  msg.SetType(STUN_BINDING_REQUEST);
  msg.SetTransactionID(
      std::string(reinterpret_cast<const char*>(kTestTransactionId2),
                  kStunTransactionIdLength));
  CheckStunTransactionID(msg, kTestTransactionId2, kStunTransactionIdLength);
  StunUInt16ListAttribute* list = StunAttribute::CreateUnknownAttributes();
  list->AddType(0x1U);
  list->AddType(0x1000U);
  list->AddType(0xAB0CU);
  EXPECT_TRUE(msg.AddAttribute(list));
  CheckStunHeader(msg, STUN_BINDING_REQUEST, (size - 20));

  talk_base::ByteBuffer out;
  EXPECT_TRUE(msg.Write(&out));
  ASSERT_EQ(size, out.Length());
  // Check everything up to the padding.
  ASSERT_EQ(0,
            memcmp(out.Data(), kStunMessageWithUInt16ListAttribute, size - 2));
}

// Test that we fail to read messages with invalid lengths.
void CheckFailureToRead(const unsigned char* testcase, size_t length) {
  StunMessage msg;
  const char* input = reinterpret_cast<const char*>(testcase);
  talk_base::ByteBuffer buf(input, length);
  ASSERT_FALSE(msg.Read(&buf));
}

TEST_F(StunTest, FailToReadInvalidMessages) {
  CheckFailureToRead(kStunMessageWithZeroLength,
                     kRealLengthOfInvalidLengthTestCases);
  CheckFailureToRead(kStunMessageWithSmallLength,
                     kRealLengthOfInvalidLengthTestCases);
  CheckFailureToRead(kStunMessageWithExcessLength,
                     kRealLengthOfInvalidLengthTestCases);
}

// Test that we properly fail to read a non-STUN message.
TEST_F(StunTest, FailToReadRtcpPacket) {
  CheckFailureToRead(kRtcpPacket, sizeof(kRtcpPacket));
}

// Check our STUN message validation code against the RFC5769 test messages.
TEST_F(StunTest, ValidateMessageIntegrity) {
  // Try the messages from RFC 5769.
  EXPECT_TRUE(StunMessage::ValidateMessageIntegrity(
      reinterpret_cast<const char*>(kRfc5769SampleRequest),
      sizeof(kRfc5769SampleRequest),
      kRfc5769SampleMsgPassword));
  EXPECT_FALSE(StunMessage::ValidateMessageIntegrity(
      reinterpret_cast<const char*>(kRfc5769SampleRequest),
      sizeof(kRfc5769SampleRequest),
      "InvalidPassword"));

  EXPECT_TRUE(StunMessage::ValidateMessageIntegrity(
      reinterpret_cast<const char*>(kRfc5769SampleResponse),
      sizeof(kRfc5769SampleResponse),
      kRfc5769SampleMsgPassword));
  EXPECT_FALSE(StunMessage::ValidateMessageIntegrity(
      reinterpret_cast<const char*>(kRfc5769SampleResponse),
      sizeof(kRfc5769SampleResponse),
      "InvalidPassword"));

  EXPECT_TRUE(StunMessage::ValidateMessageIntegrity(
      reinterpret_cast<const char*>(kRfc5769SampleResponseIPv6),
      sizeof(kRfc5769SampleResponseIPv6),
      kRfc5769SampleMsgPassword));
  EXPECT_FALSE(StunMessage::ValidateMessageIntegrity(
      reinterpret_cast<const char*>(kRfc5769SampleResponseIPv6),
      sizeof(kRfc5769SampleResponseIPv6),
      "InvalidPassword"));

  // We first need to compute the key for the long-term authentication HMAC.
  std::string key;
  ComputeStunCredentialHash(kRfc5769SampleMsgWithAuthUsername,
      kRfc5769SampleMsgWithAuthRealm, kRfc5769SampleMsgWithAuthPassword, &key);
  EXPECT_TRUE(StunMessage::ValidateMessageIntegrity(
      reinterpret_cast<const char*>(kRfc5769SampleRequestLongTermAuth),
      sizeof(kRfc5769SampleRequestLongTermAuth), key));
  EXPECT_FALSE(StunMessage::ValidateMessageIntegrity(
      reinterpret_cast<const char*>(kRfc5769SampleRequestLongTermAuth),
      sizeof(kRfc5769SampleRequestLongTermAuth),
      "InvalidPassword"));

  // Try some edge cases.
  EXPECT_FALSE(StunMessage::ValidateMessageIntegrity(
      reinterpret_cast<const char*>(kStunMessageWithZeroLength),
      sizeof(kStunMessageWithZeroLength),
      kRfc5769SampleMsgPassword));
  EXPECT_FALSE(StunMessage::ValidateMessageIntegrity(
      reinterpret_cast<const char*>(kStunMessageWithExcessLength),
      sizeof(kStunMessageWithExcessLength),
      kRfc5769SampleMsgPassword));
  EXPECT_FALSE(StunMessage::ValidateMessageIntegrity(
      reinterpret_cast<const char*>(kStunMessageWithSmallLength),
      sizeof(kStunMessageWithSmallLength),
      kRfc5769SampleMsgPassword));

  // Test that munging a single bit anywhere in the message causes the
  // message-integrity check to fail, unless it is after the M-I attribute.
  char buf[sizeof(kRfc5769SampleRequest)];
  memcpy(buf, kRfc5769SampleRequest, sizeof(kRfc5769SampleRequest));
  for (size_t i = 0; i < sizeof(buf); ++i) {
    buf[i] ^= 0x01;
    if (i > 0)
      buf[i - 1] ^= 0x01;
    EXPECT_EQ(i >= sizeof(buf) - 8, StunMessage::ValidateMessageIntegrity(
        buf, sizeof(buf), kRfc5769SampleMsgPassword));
  }
}

// Validate that we generate correct MESSAGE-INTEGRITY attributes.
// Note the use of IceMessage instead of StunMessage; this is necessary because
// the RFC5769 test messages used include attributes not found in basic STUN.
TEST_F(StunTest, AddMessageIntegrity) {
  IceMessage msg;
  talk_base::ByteBuffer buf(
      reinterpret_cast<const char*>(kRfc5769SampleRequestWithoutMI),
      sizeof(kRfc5769SampleRequestWithoutMI));
  EXPECT_TRUE(msg.Read(&buf));
  EXPECT_TRUE(msg.AddMessageIntegrity(kRfc5769SampleMsgPassword));
  const StunByteStringAttribute* mi_attr =
      msg.GetByteString(STUN_ATTR_MESSAGE_INTEGRITY);
  EXPECT_EQ(20U, mi_attr->length());
  EXPECT_EQ(0, memcmp(
      mi_attr->bytes(), kCalculatedHmac1, sizeof(kCalculatedHmac1)));

  talk_base::ByteBuffer buf1;
  EXPECT_TRUE(msg.Write(&buf1));
  EXPECT_TRUE(StunMessage::ValidateMessageIntegrity(
        reinterpret_cast<const char*>(buf1.Data()), buf1.Length(),
        kRfc5769SampleMsgPassword));

  IceMessage msg2;
  talk_base::ByteBuffer buf2(
      reinterpret_cast<const char*>(kRfc5769SampleResponseWithoutMI),
      sizeof(kRfc5769SampleResponseWithoutMI));
  EXPECT_TRUE(msg2.Read(&buf2));
  EXPECT_TRUE(msg2.AddMessageIntegrity(kRfc5769SampleMsgPassword));
  const StunByteStringAttribute* mi_attr2 =
      msg2.GetByteString(STUN_ATTR_MESSAGE_INTEGRITY);
  EXPECT_EQ(20U, mi_attr2->length());
  EXPECT_EQ(
      0, memcmp(mi_attr2->bytes(), kCalculatedHmac2, sizeof(kCalculatedHmac2)));

  talk_base::ByteBuffer buf3;
  EXPECT_TRUE(msg2.Write(&buf3));
  EXPECT_TRUE(StunMessage::ValidateMessageIntegrity(
        reinterpret_cast<const char*>(buf3.Data()), buf3.Length(),
        kRfc5769SampleMsgPassword));
}

// Check our STUN message validation code against the RFC5769 test messages.
TEST_F(StunTest, ValidateFingerprint) {
  EXPECT_TRUE(StunMessage::ValidateFingerprint(
      reinterpret_cast<const char*>(kRfc5769SampleRequest),
      sizeof(kRfc5769SampleRequest)));
  EXPECT_TRUE(StunMessage::ValidateFingerprint(
      reinterpret_cast<const char*>(kRfc5769SampleResponse),
      sizeof(kRfc5769SampleResponse)));
  EXPECT_TRUE(StunMessage::ValidateFingerprint(
      reinterpret_cast<const char*>(kRfc5769SampleResponseIPv6),
      sizeof(kRfc5769SampleResponseIPv6)));

  EXPECT_FALSE(StunMessage::ValidateFingerprint(
      reinterpret_cast<const char*>(kStunMessageWithZeroLength),
      sizeof(kStunMessageWithZeroLength)));
  EXPECT_FALSE(StunMessage::ValidateFingerprint(
      reinterpret_cast<const char*>(kStunMessageWithExcessLength),
      sizeof(kStunMessageWithExcessLength)));
  EXPECT_FALSE(StunMessage::ValidateFingerprint(
      reinterpret_cast<const char*>(kStunMessageWithSmallLength),
      sizeof(kStunMessageWithSmallLength)));

  // Test that munging a single bit anywhere in the message causes the
  // fingerprint check to fail.
  char buf[sizeof(kRfc5769SampleRequest)];
  memcpy(buf, kRfc5769SampleRequest, sizeof(kRfc5769SampleRequest));
  for (size_t i = 0; i < sizeof(buf); ++i) {
    buf[i] ^= 0x01;
    if (i > 0)
      buf[i - 1] ^= 0x01;
    EXPECT_FALSE(StunMessage::ValidateFingerprint(buf, sizeof(buf)));
  }
  // Put them all back to normal and the check should pass again.
  buf[sizeof(buf) - 1] ^= 0x01;
  EXPECT_TRUE(StunMessage::ValidateFingerprint(buf, sizeof(buf)));
}

TEST_F(StunTest, AddFingerprint) {
  IceMessage msg;
  talk_base::ByteBuffer buf(
      reinterpret_cast<const char*>(kRfc5769SampleRequestWithoutMI),
      sizeof(kRfc5769SampleRequestWithoutMI));
  EXPECT_TRUE(msg.Read(&buf));
  EXPECT_TRUE(msg.AddFingerprint());

  talk_base::ByteBuffer buf1;
  EXPECT_TRUE(msg.Write(&buf1));
  EXPECT_TRUE(StunMessage::ValidateFingerprint(
      reinterpret_cast<const char*>(buf1.Data()), buf1.Length()));
}

// Sample "GTURN" relay message.
static const unsigned char kRelayMessage[] = {
  0x00, 0x01, 0x00, 88,    // message header
  0x21, 0x12, 0xA4, 0x42,  // magic cookie
  '0', '1', '2', '3',      // transaction id
  '4', '5', '6', '7',
  '8', '9', 'a', 'b',
  0x00, 0x01, 0x00, 8,     // mapped address
  0x00, 0x01, 0x00, 13,
  0x00, 0x00, 0x00, 17,
  0x00, 0x06, 0x00, 12,    // username
  'a', 'b', 'c', 'd',
  'e', 'f', 'g', 'h',
  'i', 'j', 'k', 'l',
  0x00, 0x0d, 0x00, 4,     // lifetime
  0x00, 0x00, 0x00, 11,
  0x00, 0x0f, 0x00, 4,     // magic cookie
  0x72, 0xc6, 0x4b, 0xc6,
  0x00, 0x10, 0x00, 4,     // bandwidth
  0x00, 0x00, 0x00, 6,
  0x00, 0x11, 0x00, 8,     // destination address
  0x00, 0x01, 0x00, 13,
  0x00, 0x00, 0x00, 17,
  0x00, 0x12, 0x00, 8,     // source address 2
  0x00, 0x01, 0x00, 13,
  0x00, 0x00, 0x00, 17,
  0x00, 0x13, 0x00, 7,     // data
  'a', 'b', 'c', 'd',
  'e', 'f', 'g', 0         // DATA must be padded per rfc5766.
};

// Test that we can read the GTURN-specific fields.
TEST_F(StunTest, ReadRelayMessage) {
  RelayMessage msg, msg2;

  const char* input = reinterpret_cast<const char*>(kRelayMessage);
  size_t size = sizeof(kRelayMessage);
  talk_base::ByteBuffer buf(input, size);
  EXPECT_TRUE(msg.Read(&buf));

  EXPECT_EQ(STUN_BINDING_REQUEST, msg.type());
  EXPECT_EQ(size - 20, msg.length());
  EXPECT_EQ("0123456789ab", msg.transaction_id());

  msg2.SetType(STUN_BINDING_REQUEST);
  msg2.SetTransactionID("0123456789ab");

  in_addr legacy_in_addr;
  legacy_in_addr.s_addr = htonl(17U);
  talk_base::IPAddress legacy_ip(legacy_in_addr);

  const StunAddressAttribute* addr = msg.GetAddress(STUN_ATTR_MAPPED_ADDRESS);
  ASSERT_TRUE(addr != NULL);
  EXPECT_EQ(1, addr->family());
  EXPECT_EQ(13, addr->port());
  EXPECT_EQ(legacy_ip, addr->ipaddr());

  StunAddressAttribute* addr2 =
      StunAttribute::CreateAddress(STUN_ATTR_MAPPED_ADDRESS);
  addr2->SetPort(13);
  addr2->SetIP(legacy_ip);
  EXPECT_TRUE(msg2.AddAttribute(addr2));

  const StunByteStringAttribute* bytes = msg.GetByteString(STUN_ATTR_USERNAME);
  ASSERT_TRUE(bytes != NULL);
  EXPECT_EQ(12U, bytes->length());
  EXPECT_EQ("abcdefghijkl", bytes->GetString());

  StunByteStringAttribute* bytes2 =
  StunAttribute::CreateByteString(STUN_ATTR_USERNAME);
  bytes2->CopyBytes("abcdefghijkl");
  EXPECT_TRUE(msg2.AddAttribute(bytes2));

  const StunUInt32Attribute* uval = msg.GetUInt32(STUN_ATTR_LIFETIME);
  ASSERT_TRUE(uval != NULL);
  EXPECT_EQ(11U, uval->value());

  StunUInt32Attribute* uval2 = StunAttribute::CreateUInt32(STUN_ATTR_LIFETIME);
  uval2->SetValue(11);
  EXPECT_TRUE(msg2.AddAttribute(uval2));

  bytes = msg.GetByteString(STUN_ATTR_MAGIC_COOKIE);
  ASSERT_TRUE(bytes != NULL);
  EXPECT_EQ(4U, bytes->length());
  EXPECT_EQ(0,
            memcmp(bytes->bytes(),
                   TURN_MAGIC_COOKIE_VALUE,
                   sizeof(TURN_MAGIC_COOKIE_VALUE)));

  bytes2 = StunAttribute::CreateByteString(STUN_ATTR_MAGIC_COOKIE);
  bytes2->CopyBytes(reinterpret_cast<const char*>(TURN_MAGIC_COOKIE_VALUE),
                    sizeof(TURN_MAGIC_COOKIE_VALUE));
  EXPECT_TRUE(msg2.AddAttribute(bytes2));

  uval = msg.GetUInt32(STUN_ATTR_BANDWIDTH);
  ASSERT_TRUE(uval != NULL);
  EXPECT_EQ(6U, uval->value());

  uval2 = StunAttribute::CreateUInt32(STUN_ATTR_BANDWIDTH);
  uval2->SetValue(6);
  EXPECT_TRUE(msg2.AddAttribute(uval2));

  addr = msg.GetAddress(STUN_ATTR_DESTINATION_ADDRESS);
  ASSERT_TRUE(addr != NULL);
  EXPECT_EQ(1, addr->family());
  EXPECT_EQ(13, addr->port());
  EXPECT_EQ(legacy_ip, addr->ipaddr());

  addr2 = StunAttribute::CreateAddress(STUN_ATTR_DESTINATION_ADDRESS);
  addr2->SetPort(13);
  addr2->SetIP(legacy_ip);
  EXPECT_TRUE(msg2.AddAttribute(addr2));

  addr = msg.GetAddress(STUN_ATTR_SOURCE_ADDRESS2);
  ASSERT_TRUE(addr != NULL);
  EXPECT_EQ(1, addr->family());
  EXPECT_EQ(13, addr->port());
  EXPECT_EQ(legacy_ip, addr->ipaddr());

  addr2 = StunAttribute::CreateAddress(STUN_ATTR_SOURCE_ADDRESS2);
  addr2->SetPort(13);
  addr2->SetIP(legacy_ip);
  EXPECT_TRUE(msg2.AddAttribute(addr2));

  bytes = msg.GetByteString(STUN_ATTR_DATA);
  ASSERT_TRUE(bytes != NULL);
  EXPECT_EQ(7U, bytes->length());
  EXPECT_EQ("abcdefg", bytes->GetString());

  bytes2 = StunAttribute::CreateByteString(STUN_ATTR_DATA);
  bytes2->CopyBytes("abcdefg");
  EXPECT_TRUE(msg2.AddAttribute(bytes2));

  talk_base::ByteBuffer out;
  EXPECT_TRUE(msg.Write(&out));
  EXPECT_EQ(size, out.Length());
  size_t len1 = out.Length();
  std::string outstring;
  out.ReadString(&outstring, len1);
  EXPECT_EQ(0, memcmp(outstring.c_str(), input, len1));

  talk_base::ByteBuffer out2;
  EXPECT_TRUE(msg2.Write(&out2));
  EXPECT_EQ(size, out2.Length());
  size_t len2 = out2.Length();
  std::string outstring2;
  out2.ReadString(&outstring2, len2);
  EXPECT_EQ(0, memcmp(outstring2.c_str(), input, len2));
}

}  // namespace cricket
