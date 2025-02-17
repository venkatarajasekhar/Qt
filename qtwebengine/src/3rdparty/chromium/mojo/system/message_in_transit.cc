// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/system/message_in_transit.h"

#include <string.h>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "mojo/system/constants.h"
#include "mojo/system/transport_data.h"

namespace mojo {
namespace system {

STATIC_CONST_MEMBER_DEFINITION const MessageInTransit::Type
    MessageInTransit::kTypeMessagePipeEndpoint;
STATIC_CONST_MEMBER_DEFINITION const MessageInTransit::Type
    MessageInTransit::kTypeMessagePipe;
STATIC_CONST_MEMBER_DEFINITION const MessageInTransit::Type
    MessageInTransit::kTypeChannel;
STATIC_CONST_MEMBER_DEFINITION const MessageInTransit::Type
    MessageInTransit::kTypeRawChannel;
STATIC_CONST_MEMBER_DEFINITION const MessageInTransit::Subtype
    MessageInTransit::kSubtypeMessagePipeEndpointData;
STATIC_CONST_MEMBER_DEFINITION const MessageInTransit::Subtype
    MessageInTransit::kSubtypeChannelRunMessagePipeEndpoint;
STATIC_CONST_MEMBER_DEFINITION const MessageInTransit::Subtype
    MessageInTransit::kSubtypeChannelRemoveMessagePipeEndpoint;
STATIC_CONST_MEMBER_DEFINITION const MessageInTransit::Subtype
    MessageInTransit::kSubtypeChannelRemoveMessagePipeEndpointAck;
STATIC_CONST_MEMBER_DEFINITION const MessageInTransit::Subtype
    MessageInTransit::kSubtypeRawChannelPosixExtraPlatformHandles;
STATIC_CONST_MEMBER_DEFINITION const MessageInTransit::EndpointId
    MessageInTransit::kInvalidEndpointId;
STATIC_CONST_MEMBER_DEFINITION const size_t MessageInTransit::kMessageAlignment;

struct MessageInTransit::PrivateStructForCompileAsserts {
  // The size of |Header| must be a multiple of the alignment.
  COMPILE_ASSERT(sizeof(Header) % kMessageAlignment == 0,
                 sizeof_MessageInTransit_Header_invalid);
  // Avoid dangerous situations, but making sure that the size of the "header" +
  // the size of the data fits into a 31-bit number.
  COMPILE_ASSERT(static_cast<uint64_t>(sizeof(Header)) + kMaxMessageNumBytes <=
                     0x7fffffffULL,
                 kMaxMessageNumBytes_too_big);

  // We assume (to avoid extra rounding code) that the maximum message (data)
  // size is a multiple of the alignment.
  COMPILE_ASSERT(kMaxMessageNumBytes % kMessageAlignment == 0,
                 kMessageAlignment_not_a_multiple_of_alignment);
};

MessageInTransit::View::View(size_t message_size, const void* buffer)
    : buffer_(buffer) {
  size_t next_message_size = 0;
  DCHECK(MessageInTransit::GetNextMessageSize(buffer_, message_size,
                                              &next_message_size));
  DCHECK_EQ(message_size, next_message_size);
  // This should be equivalent.
  DCHECK_EQ(message_size, total_size());
}

bool MessageInTransit::View::IsValid(size_t serialized_platform_handle_size,
                                     const char** error_message) const {
  // Note: This also implies a check on the |main_buffer_size()|, which is just
  // |RoundUpMessageAlignment(sizeof(Header) + num_bytes())|.
  if (num_bytes() > kMaxMessageNumBytes) {
    *error_message = "Message data payload too large";
    return false;
  }

  if (transport_data_buffer_size() > 0) {
    const char* e =
        TransportData::ValidateBuffer(serialized_platform_handle_size,
                                      transport_data_buffer(),
                                      transport_data_buffer_size());
    if (e) {
      *error_message = e;
      return false;
    }
  }

  return true;
}

MessageInTransit::MessageInTransit(Type type,
                                   Subtype subtype,
                                   uint32_t num_bytes,
                                   const void* bytes)
    : main_buffer_size_(RoundUpMessageAlignment(sizeof(Header) + num_bytes)),
      main_buffer_(static_cast<char*>(base::AlignedAlloc(main_buffer_size_,
                                                         kMessageAlignment))) {
  DCHECK_LE(num_bytes, kMaxMessageNumBytes);

  // |total_size| is updated below, from the other values.
  header()->type = type;
  header()->subtype = subtype;
  header()->source_id = kInvalidEndpointId;
  header()->destination_id = kInvalidEndpointId;
  header()->num_bytes = num_bytes;
  header()->unused = 0;
  // Note: If dispatchers are subsequently attached, then |total_size| will have
  // to be adjusted.
  UpdateTotalSize();

  if (bytes) {
    memcpy(MessageInTransit::bytes(), bytes, num_bytes);
    memset(static_cast<char*>(MessageInTransit::bytes()) + num_bytes, 0,
           main_buffer_size_ - sizeof(Header) - num_bytes);
  } else {
    memset(MessageInTransit::bytes(), 0, main_buffer_size_ - sizeof(Header));
  }
}

MessageInTransit::MessageInTransit(const View& message_view)
    : main_buffer_size_(message_view.main_buffer_size()),
      main_buffer_(static_cast<char*>(base::AlignedAlloc(main_buffer_size_,
                                                         kMessageAlignment))) {
  DCHECK_GE(main_buffer_size_, sizeof(Header));
  DCHECK_EQ(main_buffer_size_ % kMessageAlignment, 0u);

  memcpy(main_buffer_.get(), message_view.main_buffer(), main_buffer_size_);
  DCHECK_EQ(main_buffer_size_,
            RoundUpMessageAlignment(sizeof(Header) + num_bytes()));
}

MessageInTransit::~MessageInTransit() {
  if (dispatchers_) {
    for (size_t i = 0; i < dispatchers_->size(); i++) {
      if (!(*dispatchers_)[i])
        continue;

      DCHECK((*dispatchers_)[i]->HasOneRef());
      (*dispatchers_)[i]->Close();
    }
  }
}

// static
bool MessageInTransit::GetNextMessageSize(const void* buffer,
                                          size_t buffer_size,
                                          size_t* next_message_size) {
  DCHECK(next_message_size);
  if (!buffer_size)
    return false;
  DCHECK(buffer);
  DCHECK_EQ(reinterpret_cast<uintptr_t>(buffer) %
                MessageInTransit::kMessageAlignment, 0u);

  if (buffer_size < sizeof(Header))
    return false;

  const Header* header = static_cast<const Header*>(buffer);
  *next_message_size = header->total_size;
  DCHECK_EQ(*next_message_size % kMessageAlignment, 0u);
  return true;
}

void MessageInTransit::SetDispatchers(
    scoped_ptr<DispatcherVector> dispatchers) {
  DCHECK(dispatchers);
  DCHECK(!dispatchers_);
  DCHECK(!transport_data_);

  dispatchers_ = dispatchers.Pass();
#ifndef NDEBUG
  for (size_t i = 0; i < dispatchers_->size(); i++)
    DCHECK(!(*dispatchers_)[i] || (*dispatchers_)[i]->HasOneRef());
#endif
}

void MessageInTransit::SetTransportData(
    scoped_ptr<TransportData> transport_data) {
  DCHECK(transport_data);
  DCHECK(!transport_data_);
  DCHECK(!dispatchers_);

  transport_data_ = transport_data.Pass();
}

void MessageInTransit::SerializeAndCloseDispatchers(Channel* channel) {
  DCHECK(channel);
  DCHECK(!transport_data_);

  if (!dispatchers_ || !dispatchers_->size())
    return;

  transport_data_.reset(new TransportData(dispatchers_.Pass(), channel));

  // Update the sizes in the message header.
  UpdateTotalSize();
}

void MessageInTransit::UpdateTotalSize() {
  DCHECK_EQ(main_buffer_size_ % kMessageAlignment, 0u);
  header()->total_size = static_cast<uint32_t>(main_buffer_size_);
  if (transport_data_) {
    header()->total_size +=
        static_cast<uint32_t>(transport_data_->buffer_size());
  }
}

}  // namespace system
}  // namespace mojo
