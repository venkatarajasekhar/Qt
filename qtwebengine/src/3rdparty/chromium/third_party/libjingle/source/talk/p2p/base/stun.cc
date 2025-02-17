/*
 * libjingle
 * Copyright 2004--2005, Google Inc.
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

#include "talk/p2p/base/stun.h"

#include <string.h>

#include "talk/base/byteorder.h"
#include "talk/base/common.h"
#include "talk/base/crc32.h"
#include "talk/base/logging.h"
#include "talk/base/messagedigest.h"
#include "talk/base/scoped_ptr.h"
#include "talk/base/stringencode.h"

using talk_base::ByteBuffer;

namespace cricket {

const char STUN_ERROR_REASON_BAD_REQUEST[] = "Bad Request";
const char STUN_ERROR_REASON_UNAUTHORIZED[] = "Unauthorized";
const char STUN_ERROR_REASON_FORBIDDEN[] = "Forbidden";
const char STUN_ERROR_REASON_STALE_CREDENTIALS[] = "Stale Credentials";
const char STUN_ERROR_REASON_ALLOCATION_MISMATCH[] = "Allocation Mismatch";
const char STUN_ERROR_REASON_STALE_NONCE[] = "Stale Nonce";
const char STUN_ERROR_REASON_WRONG_CREDENTIALS[] = "Wrong Credentials";
const char STUN_ERROR_REASON_UNSUPPORTED_PROTOCOL[] = "Unsupported Protocol";
const char STUN_ERROR_REASON_ROLE_CONFLICT[] = "Role Conflict";
const char STUN_ERROR_REASON_SERVER_ERROR[] = "Server Error";

const char TURN_MAGIC_COOKIE_VALUE[] = { '\x72', '\xC6', '\x4B', '\xC6' };
const char EMPTY_TRANSACTION_ID[] = "0000000000000000";
const uint32 STUN_FINGERPRINT_XOR_VALUE = 0x5354554E;

// StunMessage

StunMessage::StunMessage()
    : type_(0),
      length_(0),
      transaction_id_(EMPTY_TRANSACTION_ID) {
  ASSERT(IsValidTransactionId(transaction_id_));
  attrs_ = new std::vector<StunAttribute*>();
}

StunMessage::~StunMessage() {
  for (size_t i = 0; i < attrs_->size(); i++)
    delete (*attrs_)[i];
  delete attrs_;
}

bool StunMessage::IsLegacy() const {
  if (transaction_id_.size() == kStunLegacyTransactionIdLength)
    return true;
  ASSERT(transaction_id_.size() == kStunTransactionIdLength);
  return false;
}

bool StunMessage::SetTransactionID(const std::string& str) {
  if (!IsValidTransactionId(str)) {
    return false;
  }
  transaction_id_ = str;
  return true;
}

bool StunMessage::AddAttribute(StunAttribute* attr) {
  // Fail any attributes that aren't valid for this type of message.
  if (attr->value_type() != GetAttributeValueType(attr->type())) {
    return false;
  }
  attrs_->push_back(attr);
  attr->SetOwner(this);
  size_t attr_length = attr->length();
  if (attr_length % 4 != 0) {
    attr_length += (4 - (attr_length % 4));
  }
  length_ += static_cast<uint16>(attr_length + 4);
  return true;
}

const StunAddressAttribute* StunMessage::GetAddress(int type) const {
  switch (type) {
    case STUN_ATTR_MAPPED_ADDRESS: {
      // Return XOR-MAPPED-ADDRESS when MAPPED-ADDRESS attribute is
      // missing.
      const StunAttribute* mapped_address =
          GetAttribute(STUN_ATTR_MAPPED_ADDRESS);
      if (!mapped_address)
        mapped_address = GetAttribute(STUN_ATTR_XOR_MAPPED_ADDRESS);
      return reinterpret_cast<const StunAddressAttribute*>(mapped_address);
    }

    default:
      return static_cast<const StunAddressAttribute*>(GetAttribute(type));
  }
}

const StunUInt32Attribute* StunMessage::GetUInt32(int type) const {
  return static_cast<const StunUInt32Attribute*>(GetAttribute(type));
}

const StunUInt64Attribute* StunMessage::GetUInt64(int type) const {
  return static_cast<const StunUInt64Attribute*>(GetAttribute(type));
}

const StunByteStringAttribute* StunMessage::GetByteString(int type) const {
  return static_cast<const StunByteStringAttribute*>(GetAttribute(type));
}

const StunErrorCodeAttribute* StunMessage::GetErrorCode() const {
  return static_cast<const StunErrorCodeAttribute*>(
      GetAttribute(STUN_ATTR_ERROR_CODE));
}

const StunUInt16ListAttribute* StunMessage::GetUnknownAttributes() const {
  return static_cast<const StunUInt16ListAttribute*>(
      GetAttribute(STUN_ATTR_UNKNOWN_ATTRIBUTES));
}

// Verifies a STUN message has a valid MESSAGE-INTEGRITY attribute, using the
// procedure outlined in RFC 5389, section 15.4.
bool StunMessage::ValidateMessageIntegrity(const char* data, size_t size,
                                           const std::string& password) {
  // Verifying the size of the message.
  if ((size % 4) != 0) {
    return false;
  }

  // Getting the message length from the STUN header.
  uint16 msg_length = talk_base::GetBE16(&data[2]);
  if (size != (msg_length + kStunHeaderSize)) {
    return false;
  }

  // Finding Message Integrity attribute in stun message.
  size_t current_pos = kStunHeaderSize;
  bool has_message_integrity_attr = false;
  while (current_pos < size) {
    uint16 attr_type, attr_length;
    // Getting attribute type and length.
    attr_type = talk_base::GetBE16(&data[current_pos]);
    attr_length = talk_base::GetBE16(&data[current_pos + sizeof(attr_type)]);

    // If M-I, sanity check it, and break out.
    if (attr_type == STUN_ATTR_MESSAGE_INTEGRITY) {
      if (attr_length != kStunMessageIntegritySize ||
          current_pos + attr_length > size) {
        return false;
      }
      has_message_integrity_attr = true;
      break;
    }

    // Otherwise, skip to the next attribute.
    current_pos += sizeof(attr_type) + sizeof(attr_length) + attr_length;
    if ((attr_length % 4) != 0) {
      current_pos += (4 - (attr_length % 4));
    }
  }

  if (!has_message_integrity_attr) {
    return false;
  }

  // Getting length of the message to calculate Message Integrity.
  size_t mi_pos = current_pos;
  talk_base::scoped_ptr<char[]> temp_data(new char[current_pos]);
  memcpy(temp_data.get(), data, current_pos);
  if (size > mi_pos + kStunAttributeHeaderSize + kStunMessageIntegritySize) {
    // Stun message has other attributes after message integrity.
    // Adjust the length parameter in stun message to calculate HMAC.
    size_t extra_offset = size -
        (mi_pos + kStunAttributeHeaderSize + kStunMessageIntegritySize);
    size_t new_adjusted_len = size - extra_offset - kStunHeaderSize;

    // Writing new length of the STUN message @ Message Length in temp buffer.
    //      0                   1                   2                   3
    //      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //     |0 0|     STUN Message Type     |         Message Length        |
    //     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    talk_base::SetBE16(temp_data.get() + 2,
                       static_cast<uint16>(new_adjusted_len));
  }

  char hmac[kStunMessageIntegritySize];
  size_t ret = talk_base::ComputeHmac(talk_base::DIGEST_SHA_1,
                                      password.c_str(), password.size(),
                                      temp_data.get(), mi_pos,
                                      hmac, sizeof(hmac));
  ASSERT(ret == sizeof(hmac));
  if (ret != sizeof(hmac))
    return false;

  // Comparing the calculated HMAC with the one present in the message.
  return memcmp(data + current_pos + kStunAttributeHeaderSize,
                hmac,
                sizeof(hmac)) == 0;
}

bool StunMessage::AddMessageIntegrity(const std::string& password) {
  return AddMessageIntegrity(password.c_str(), password.size());
}

bool StunMessage::AddMessageIntegrity(const char* key,
                                      size_t keylen) {
  // Add the attribute with a dummy value. Since this is a known attribute, it
  // can't fail.
  StunByteStringAttribute* msg_integrity_attr =
      new StunByteStringAttribute(STUN_ATTR_MESSAGE_INTEGRITY,
          std::string(kStunMessageIntegritySize, '0'));
  VERIFY(AddAttribute(msg_integrity_attr));

  // Calculate the HMAC for the message.
  talk_base::ByteBuffer buf;
  if (!Write(&buf))
    return false;

  int msg_len_for_hmac = static_cast<int>(
      buf.Length() - kStunAttributeHeaderSize - msg_integrity_attr->length());
  char hmac[kStunMessageIntegritySize];
  size_t ret = talk_base::ComputeHmac(talk_base::DIGEST_SHA_1,
                                      key, keylen,
                                      buf.Data(), msg_len_for_hmac,
                                      hmac, sizeof(hmac));
  ASSERT(ret == sizeof(hmac));
  if (ret != sizeof(hmac)) {
    LOG(LS_ERROR) << "HMAC computation failed. Message-Integrity "
                  << "has dummy value.";
    return false;
  }

  // Insert correct HMAC into the attribute.
  msg_integrity_attr->CopyBytes(hmac, sizeof(hmac));
  return true;
}

// Verifies a message is in fact a STUN message, by performing the checks
// outlined in RFC 5389, section 7.3, including the FINGERPRINT check detailed
// in section 15.5.
bool StunMessage::ValidateFingerprint(const char* data, size_t size) {
  // Check the message length.
  size_t fingerprint_attr_size =
      kStunAttributeHeaderSize + StunUInt32Attribute::SIZE;
  if (size % 4 != 0 || size < kStunHeaderSize + fingerprint_attr_size)
    return false;

  // Skip the rest if the magic cookie isn't present.
  const char* magic_cookie =
      data + kStunTransactionIdOffset - kStunMagicCookieLength;
  if (talk_base::GetBE32(magic_cookie) != kStunMagicCookie)
    return false;

  // Check the fingerprint type and length.
  const char* fingerprint_attr_data = data + size - fingerprint_attr_size;
  if (talk_base::GetBE16(fingerprint_attr_data) != STUN_ATTR_FINGERPRINT ||
      talk_base::GetBE16(fingerprint_attr_data + sizeof(uint16)) !=
          StunUInt32Attribute::SIZE)
    return false;

  // Check the fingerprint value.
  uint32 fingerprint =
      talk_base::GetBE32(fingerprint_attr_data + kStunAttributeHeaderSize);
  return ((fingerprint ^ STUN_FINGERPRINT_XOR_VALUE) ==
      talk_base::ComputeCrc32(data, size - fingerprint_attr_size));
}

bool StunMessage::AddFingerprint() {
  // Add the attribute with a dummy value. Since this is a known attribute,
  // it can't fail.
  StunUInt32Attribute* fingerprint_attr =
     new StunUInt32Attribute(STUN_ATTR_FINGERPRINT, 0);
  VERIFY(AddAttribute(fingerprint_attr));

  // Calculate the CRC-32 for the message and insert it.
  talk_base::ByteBuffer buf;
  if (!Write(&buf))
    return false;

  int msg_len_for_crc32 = static_cast<int>(
      buf.Length() - kStunAttributeHeaderSize - fingerprint_attr->length());
  uint32 c = talk_base::ComputeCrc32(buf.Data(), msg_len_for_crc32);

  // Insert the correct CRC-32, XORed with a constant, into the attribute.
  fingerprint_attr->SetValue(c ^ STUN_FINGERPRINT_XOR_VALUE);
  return true;
}

bool StunMessage::Read(ByteBuffer* buf) {
  if (!buf->ReadUInt16(&type_))
    return false;

  if (type_ & 0x8000) {
    // RTP and RTCP set the MSB of first byte, since first two bits are version,
    // and version is always 2 (10). If set, this is not a STUN packet.
    return false;
  }

  if (!buf->ReadUInt16(&length_))
    return false;

  std::string magic_cookie;
  if (!buf->ReadString(&magic_cookie, kStunMagicCookieLength))
    return false;

  std::string transaction_id;
  if (!buf->ReadString(&transaction_id, kStunTransactionIdLength))
    return false;

  uint32 magic_cookie_int =
      *reinterpret_cast<const uint32*>(magic_cookie.data());
  if (talk_base::NetworkToHost32(magic_cookie_int) != kStunMagicCookie) {
    // If magic cookie is invalid it means that the peer implements
    // RFC3489 instead of RFC5389.
    transaction_id.insert(0, magic_cookie);
  }
  ASSERT(IsValidTransactionId(transaction_id));
  transaction_id_ = transaction_id;

  if (length_ != buf->Length())
    return false;

  attrs_->resize(0);

  size_t rest = buf->Length() - length_;
  while (buf->Length() > rest) {
    uint16 attr_type, attr_length;
    if (!buf->ReadUInt16(&attr_type))
      return false;
    if (!buf->ReadUInt16(&attr_length))
      return false;

    StunAttribute* attr = CreateAttribute(attr_type, attr_length);
    if (!attr) {
      // Skip any unknown or malformed attributes.
      if ((attr_length % 4) != 0) {
        attr_length += (4 - (attr_length % 4));
      }
      if (!buf->Consume(attr_length))
        return false;
    } else {
      if (!attr->Read(buf))
        return false;
      attrs_->push_back(attr);
    }
  }

  ASSERT(buf->Length() == rest);
  return true;
}

bool StunMessage::Write(ByteBuffer* buf) const {
  buf->WriteUInt16(type_);
  buf->WriteUInt16(length_);
  if (!IsLegacy())
    buf->WriteUInt32(kStunMagicCookie);
  buf->WriteString(transaction_id_);

  for (size_t i = 0; i < attrs_->size(); ++i) {
    buf->WriteUInt16((*attrs_)[i]->type());
    buf->WriteUInt16(static_cast<uint16>((*attrs_)[i]->length()));
    if (!(*attrs_)[i]->Write(buf))
      return false;
  }

  return true;
}

StunAttributeValueType StunMessage::GetAttributeValueType(int type) const {
  switch (type) {
    case STUN_ATTR_MAPPED_ADDRESS:      return STUN_VALUE_ADDRESS;
    case STUN_ATTR_USERNAME:            return STUN_VALUE_BYTE_STRING;
    case STUN_ATTR_MESSAGE_INTEGRITY:   return STUN_VALUE_BYTE_STRING;
    case STUN_ATTR_ERROR_CODE:          return STUN_VALUE_ERROR_CODE;
    case STUN_ATTR_UNKNOWN_ATTRIBUTES:  return STUN_VALUE_UINT16_LIST;
    case STUN_ATTR_REALM:               return STUN_VALUE_BYTE_STRING;
    case STUN_ATTR_NONCE:               return STUN_VALUE_BYTE_STRING;
    case STUN_ATTR_XOR_MAPPED_ADDRESS:  return STUN_VALUE_XOR_ADDRESS;
    case STUN_ATTR_SOFTWARE:            return STUN_VALUE_BYTE_STRING;
    case STUN_ATTR_ALTERNATE_SERVER:    return STUN_VALUE_BYTE_STRING;
    case STUN_ATTR_FINGERPRINT:         return STUN_VALUE_UINT32;
    case STUN_ATTR_RETRANSMIT_COUNT:    return STUN_VALUE_UINT32;
    default:                            return STUN_VALUE_UNKNOWN;
  }
}

StunAttribute* StunMessage::CreateAttribute(int type, size_t length) /*const*/ {
  StunAttributeValueType value_type = GetAttributeValueType(type);
  return StunAttribute::Create(value_type, type,
                               static_cast<uint16>(length), this);
}

const StunAttribute* StunMessage::GetAttribute(int type) const {
  for (size_t i = 0; i < attrs_->size(); ++i) {
    if ((*attrs_)[i]->type() == type)
      return (*attrs_)[i];
  }
  return NULL;
}

bool StunMessage::IsValidTransactionId(const std::string& transaction_id) {
  return transaction_id.size() == kStunTransactionIdLength ||
      transaction_id.size() == kStunLegacyTransactionIdLength;
}

// StunAttribute

StunAttribute::StunAttribute(uint16 type, uint16 length)
    : type_(type), length_(length) {
}

void StunAttribute::ConsumePadding(talk_base::ByteBuffer* buf) const {
  int remainder = length_ % 4;
  if (remainder > 0) {
    buf->Consume(4 - remainder);
  }
}

void StunAttribute::WritePadding(talk_base::ByteBuffer* buf) const {
  int remainder = length_ % 4;
  if (remainder > 0) {
    char zeroes[4] = {0};
    buf->WriteBytes(zeroes, 4 - remainder);
  }
}

StunAttribute* StunAttribute::Create(StunAttributeValueType value_type,
                                     uint16 type, uint16 length,
                                     StunMessage* owner) {
  switch (value_type) {
    case STUN_VALUE_ADDRESS:
      return new StunAddressAttribute(type, length);
    case STUN_VALUE_XOR_ADDRESS:
      return new StunXorAddressAttribute(type, length, owner);
    case STUN_VALUE_UINT32:
      return new StunUInt32Attribute(type);
    case STUN_VALUE_UINT64:
      return new StunUInt64Attribute(type);
    case STUN_VALUE_BYTE_STRING:
      return new StunByteStringAttribute(type, length);
    case STUN_VALUE_ERROR_CODE:
      return new StunErrorCodeAttribute(type, length);
    case STUN_VALUE_UINT16_LIST:
      return new StunUInt16ListAttribute(type, length);
    default:
      return NULL;
  }
}

StunAddressAttribute* StunAttribute::CreateAddress(uint16 type) {
  return new StunAddressAttribute(type, 0);
}

StunXorAddressAttribute* StunAttribute::CreateXorAddress(uint16 type) {
  return new StunXorAddressAttribute(type, 0, NULL);
}

StunUInt64Attribute* StunAttribute::CreateUInt64(uint16 type) {
  return new StunUInt64Attribute(type);
}

StunUInt32Attribute* StunAttribute::CreateUInt32(uint16 type) {
  return new StunUInt32Attribute(type);
}

StunByteStringAttribute* StunAttribute::CreateByteString(uint16 type) {
  return new StunByteStringAttribute(type, 0);
}

StunErrorCodeAttribute* StunAttribute::CreateErrorCode() {
  return new StunErrorCodeAttribute(
      STUN_ATTR_ERROR_CODE, StunErrorCodeAttribute::MIN_SIZE);
}

StunUInt16ListAttribute* StunAttribute::CreateUnknownAttributes() {
  return new StunUInt16ListAttribute(STUN_ATTR_UNKNOWN_ATTRIBUTES, 0);
}

StunAddressAttribute::StunAddressAttribute(uint16 type,
   const talk_base::SocketAddress& addr)
   : StunAttribute(type, 0) {
  SetAddress(addr);
}

StunAddressAttribute::StunAddressAttribute(uint16 type, uint16 length)
    : StunAttribute(type, length) {
}

bool StunAddressAttribute::Read(ByteBuffer* buf) {
  uint8 dummy;
  if (!buf->ReadUInt8(&dummy))
    return false;

  uint8 stun_family;
  if (!buf->ReadUInt8(&stun_family)) {
    return false;
  }
  uint16 port;
  if (!buf->ReadUInt16(&port))
    return false;
  if (stun_family == STUN_ADDRESS_IPV4) {
    in_addr v4addr;
    if (length() != SIZE_IP4) {
      return false;
    }
    if (!buf->ReadBytes(reinterpret_cast<char*>(&v4addr), sizeof(v4addr))) {
      return false;
    }
    talk_base::IPAddress ipaddr(v4addr);
    SetAddress(talk_base::SocketAddress(ipaddr, port));
  } else if (stun_family == STUN_ADDRESS_IPV6) {
    in6_addr v6addr;
    if (length() != SIZE_IP6) {
      return false;
    }
    if (!buf->ReadBytes(reinterpret_cast<char*>(&v6addr), sizeof(v6addr))) {
      return false;
    }
    talk_base::IPAddress ipaddr(v6addr);
    SetAddress(talk_base::SocketAddress(ipaddr, port));
  } else {
    return false;
  }
  return true;
}

bool StunAddressAttribute::Write(ByteBuffer* buf) const {
  StunAddressFamily address_family = family();
  if (address_family == STUN_ADDRESS_UNDEF) {
    LOG(LS_ERROR) << "Error writing address attribute: unknown family.";
    return false;
  }
  buf->WriteUInt8(0);
  buf->WriteUInt8(address_family);
  buf->WriteUInt16(address_.port());
  switch (address_.family()) {
    case AF_INET: {
      in_addr v4addr = address_.ipaddr().ipv4_address();
      buf->WriteBytes(reinterpret_cast<char*>(&v4addr), sizeof(v4addr));
      break;
    }
    case AF_INET6: {
      in6_addr v6addr = address_.ipaddr().ipv6_address();
      buf->WriteBytes(reinterpret_cast<char*>(&v6addr), sizeof(v6addr));
      break;
    }
  }
  return true;
}

StunXorAddressAttribute::StunXorAddressAttribute(uint16 type,
    const talk_base::SocketAddress& addr)
    : StunAddressAttribute(type, addr), owner_(NULL) {
}

StunXorAddressAttribute::StunXorAddressAttribute(uint16 type,
                                                 uint16 length,
                                                 StunMessage* owner)
    : StunAddressAttribute(type, length), owner_(owner) {}

talk_base::IPAddress StunXorAddressAttribute::GetXoredIP() const {
  if (owner_) {
    talk_base::IPAddress ip = ipaddr();
    switch (ip.family()) {
      case AF_INET: {
        in_addr v4addr = ip.ipv4_address();
        v4addr.s_addr =
            (v4addr.s_addr ^ talk_base::HostToNetwork32(kStunMagicCookie));
        return talk_base::IPAddress(v4addr);
      }
      case AF_INET6: {
        in6_addr v6addr = ip.ipv6_address();
        const std::string& transaction_id = owner_->transaction_id();
        if (transaction_id.length() == kStunTransactionIdLength) {
          uint32 transactionid_as_ints[3];
          memcpy(&transactionid_as_ints[0], transaction_id.c_str(),
                 transaction_id.length());
          uint32* ip_as_ints = reinterpret_cast<uint32*>(&v6addr.s6_addr);
          // Transaction ID is in network byte order, but magic cookie
          // is stored in host byte order.
          ip_as_ints[0] =
              (ip_as_ints[0] ^ talk_base::HostToNetwork32(kStunMagicCookie));
          ip_as_ints[1] = (ip_as_ints[1] ^ transactionid_as_ints[0]);
          ip_as_ints[2] = (ip_as_ints[2] ^ transactionid_as_ints[1]);
          ip_as_ints[3] = (ip_as_ints[3] ^ transactionid_as_ints[2]);
          return talk_base::IPAddress(v6addr);
        }
        break;
      }
    }
  }
  // Invalid ip family or transaction ID, or missing owner.
  // Return an AF_UNSPEC address.
  return talk_base::IPAddress();
}

bool StunXorAddressAttribute::Read(ByteBuffer* buf) {
  if (!StunAddressAttribute::Read(buf))
    return false;
  uint16 xoredport = port() ^ (kStunMagicCookie >> 16);
  talk_base::IPAddress xored_ip = GetXoredIP();
  SetAddress(talk_base::SocketAddress(xored_ip, xoredport));
  return true;
}

bool StunXorAddressAttribute::Write(ByteBuffer* buf) const {
  StunAddressFamily address_family = family();
  if (address_family == STUN_ADDRESS_UNDEF) {
    LOG(LS_ERROR) << "Error writing xor-address attribute: unknown family.";
    return false;
  }
  talk_base::IPAddress xored_ip = GetXoredIP();
  if (xored_ip.family() == AF_UNSPEC) {
    return false;
  }
  buf->WriteUInt8(0);
  buf->WriteUInt8(family());
  buf->WriteUInt16(port() ^ (kStunMagicCookie >> 16));
  switch (xored_ip.family()) {
    case AF_INET: {
      in_addr v4addr = xored_ip.ipv4_address();
      buf->WriteBytes(reinterpret_cast<const char*>(&v4addr), sizeof(v4addr));
      break;
    }
    case AF_INET6: {
      in6_addr v6addr = xored_ip.ipv6_address();
      buf->WriteBytes(reinterpret_cast<const char*>(&v6addr), sizeof(v6addr));
      break;
    }
  }
  return true;
}

StunUInt32Attribute::StunUInt32Attribute(uint16 type, uint32 value)
    : StunAttribute(type, SIZE), bits_(value) {
}

StunUInt32Attribute::StunUInt32Attribute(uint16 type)
    : StunAttribute(type, SIZE), bits_(0) {
}

bool StunUInt32Attribute::GetBit(size_t index) const {
  ASSERT(index < 32);
  return static_cast<bool>((bits_ >> index) & 0x1);
}

void StunUInt32Attribute::SetBit(size_t index, bool value) {
  ASSERT(index < 32);
  bits_ &= ~(1 << index);
  bits_ |= value ? (1 << index) : 0;
}

bool StunUInt32Attribute::Read(ByteBuffer* buf) {
  if (length() != SIZE || !buf->ReadUInt32(&bits_))
    return false;
  return true;
}

bool StunUInt32Attribute::Write(ByteBuffer* buf) const {
  buf->WriteUInt32(bits_);
  return true;
}

StunUInt64Attribute::StunUInt64Attribute(uint16 type, uint64 value)
    : StunAttribute(type, SIZE), bits_(value) {
}

StunUInt64Attribute::StunUInt64Attribute(uint16 type)
    : StunAttribute(type, SIZE), bits_(0) {
}

bool StunUInt64Attribute::Read(ByteBuffer* buf) {
  if (length() != SIZE || !buf->ReadUInt64(&bits_))
    return false;
  return true;
}

bool StunUInt64Attribute::Write(ByteBuffer* buf) const {
  buf->WriteUInt64(bits_);
  return true;
}

StunByteStringAttribute::StunByteStringAttribute(uint16 type)
    : StunAttribute(type, 0), bytes_(NULL) {
}

StunByteStringAttribute::StunByteStringAttribute(uint16 type,
                                                 const std::string& str)
    : StunAttribute(type, 0), bytes_(NULL) {
  CopyBytes(str.c_str(), str.size());
}

StunByteStringAttribute::StunByteStringAttribute(uint16 type,
                                                 const void* bytes,
                                                 size_t length)
    : StunAttribute(type, 0), bytes_(NULL) {
  CopyBytes(bytes, length);
}

StunByteStringAttribute::StunByteStringAttribute(uint16 type, uint16 length)
    : StunAttribute(type, length), bytes_(NULL) {
}

StunByteStringAttribute::~StunByteStringAttribute() {
  delete [] bytes_;
}

void StunByteStringAttribute::CopyBytes(const char* bytes) {
  CopyBytes(bytes, strlen(bytes));
}

void StunByteStringAttribute::CopyBytes(const void* bytes, size_t length) {
  char* new_bytes = new char[length];
  memcpy(new_bytes, bytes, length);
  SetBytes(new_bytes, length);
}

uint8 StunByteStringAttribute::GetByte(size_t index) const {
  ASSERT(bytes_ != NULL);
  ASSERT(index < length());
  return static_cast<uint8>(bytes_[index]);
}

void StunByteStringAttribute::SetByte(size_t index, uint8 value) {
  ASSERT(bytes_ != NULL);
  ASSERT(index < length());
  bytes_[index] = value;
}

bool StunByteStringAttribute::Read(ByteBuffer* buf) {
  bytes_ = new char[length()];
  if (!buf->ReadBytes(bytes_, length())) {
    return false;
  }

  ConsumePadding(buf);
  return true;
}

bool StunByteStringAttribute::Write(ByteBuffer* buf) const {
  buf->WriteBytes(bytes_, length());
  WritePadding(buf);
  return true;
}

void StunByteStringAttribute::SetBytes(char* bytes, size_t length) {
  delete [] bytes_;
  bytes_ = bytes;
  SetLength(static_cast<uint16>(length));
}

StunErrorCodeAttribute::StunErrorCodeAttribute(uint16 type, int code,
                                               const std::string& reason)
    : StunAttribute(type, 0) {
  SetCode(code);
  SetReason(reason);
}

StunErrorCodeAttribute::StunErrorCodeAttribute(uint16 type, uint16 length)
    : StunAttribute(type, length), class_(0), number_(0) {
}

StunErrorCodeAttribute::~StunErrorCodeAttribute() {
}

int StunErrorCodeAttribute::code() const {
  return class_ * 100 + number_;
}

void StunErrorCodeAttribute::SetCode(int code) {
  class_ = static_cast<uint8>(code / 100);
  number_ = static_cast<uint8>(code % 100);
}

void StunErrorCodeAttribute::SetReason(const std::string& reason) {
  SetLength(MIN_SIZE + static_cast<uint16>(reason.size()));
  reason_ = reason;
}

bool StunErrorCodeAttribute::Read(ByteBuffer* buf) {
  uint32 val;
  if (length() < MIN_SIZE || !buf->ReadUInt32(&val))
    return false;

  if ((val >> 11) != 0)
    LOG(LS_ERROR) << "error-code bits not zero";

  class_ = ((val >> 8) & 0x7);
  number_ = (val & 0xff);

  if (!buf->ReadString(&reason_, length() - 4))
    return false;

  ConsumePadding(buf);
  return true;
}

bool StunErrorCodeAttribute::Write(ByteBuffer* buf) const {
  buf->WriteUInt32(class_ << 8 | number_);
  buf->WriteString(reason_);
  WritePadding(buf);
  return true;
}

StunUInt16ListAttribute::StunUInt16ListAttribute(uint16 type, uint16 length)
    : StunAttribute(type, length) {
  attr_types_ = new std::vector<uint16>();
}

StunUInt16ListAttribute::~StunUInt16ListAttribute() {
  delete attr_types_;
}

size_t StunUInt16ListAttribute::Size() const {
  return attr_types_->size();
}

uint16 StunUInt16ListAttribute::GetType(int index) const {
  return (*attr_types_)[index];
}

void StunUInt16ListAttribute::SetType(int index, uint16 value) {
  (*attr_types_)[index] = value;
}

void StunUInt16ListAttribute::AddType(uint16 value) {
  attr_types_->push_back(value);
  SetLength(static_cast<uint16>(attr_types_->size() * 2));
}

bool StunUInt16ListAttribute::Read(ByteBuffer* buf) {
  if (length() % 2)
    return false;

  for (size_t i = 0; i < length() / 2; i++) {
    uint16 attr;
    if (!buf->ReadUInt16(&attr))
      return false;
    attr_types_->push_back(attr);
  }
  // Padding of these attributes is done in RFC 5389 style. This is
  // slightly different from RFC3489, but it shouldn't be important.
  // RFC3489 pads out to a 32 bit boundary by duplicating one of the
  // entries in the list (not necessarily the last one - it's unspecified).
  // RFC5389 pads on the end, and the bytes are always ignored.
  ConsumePadding(buf);
  return true;
}

bool StunUInt16ListAttribute::Write(ByteBuffer* buf) const {
  for (size_t i = 0; i < attr_types_->size(); ++i) {
    buf->WriteUInt16((*attr_types_)[i]);
  }
  WritePadding(buf);
  return true;
}

int GetStunSuccessResponseType(int req_type) {
  return IsStunRequestType(req_type) ? (req_type | 0x100) : -1;
}

int GetStunErrorResponseType(int req_type) {
  return IsStunRequestType(req_type) ? (req_type | 0x110) : -1;
}

bool IsStunRequestType(int msg_type) {
  return ((msg_type & kStunTypeMask) == 0x000);
}

bool IsStunIndicationType(int msg_type) {
  return ((msg_type & kStunTypeMask) == 0x010);
}

bool IsStunSuccessResponseType(int msg_type) {
  return ((msg_type & kStunTypeMask) == 0x100);
}

bool IsStunErrorResponseType(int msg_type) {
  return ((msg_type & kStunTypeMask) == 0x110);
}

bool ComputeStunCredentialHash(const std::string& username,
                               const std::string& realm,
                               const std::string& password,
                               std::string* hash) {
  // http://tools.ietf.org/html/rfc5389#section-15.4
  // long-term credentials will be calculated using the key and key is
  // key = MD5(username ":" realm ":" SASLprep(password))
  std::string input = username;
  input += ':';
  input += realm;
  input += ':';
  input += password;

  char digest[talk_base::MessageDigest::kMaxSize];
  size_t size = talk_base::ComputeDigest(
      talk_base::DIGEST_MD5, input.c_str(), input.size(),
      digest, sizeof(digest));
  if (size == 0) {
    return false;
  }

  *hash = std::string(digest, size);
  return true;
}

}  // namespace cricket
