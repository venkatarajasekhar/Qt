// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/formats/mp4/box_reader.h"

#include <string.h>
#include <algorithm>
#include <map>
#include <set>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "media/formats/mp4/box_definitions.h"
#include "media/formats/mp4/rcheck.h"

namespace media {
namespace mp4 {

Box::~Box() {}

bool BufferReader::Read1(uint8* v) {
  RCHECK(HasBytes(1));
  *v = buf_[pos_++];
  return true;
}

// Internal implementation of multi-byte reads
template<typename T> bool BufferReader::Read(T* v) {
  RCHECK(HasBytes(sizeof(T)));

  T tmp = 0;
  for (size_t i = 0; i < sizeof(T); i++) {
    tmp <<= 8;
    tmp += buf_[pos_++];
  }
  *v = tmp;
  return true;
}

bool BufferReader::Read2(uint16* v) { return Read(v); }
bool BufferReader::Read2s(int16* v) { return Read(v); }
bool BufferReader::Read4(uint32* v) { return Read(v); }
bool BufferReader::Read4s(int32* v) { return Read(v); }
bool BufferReader::Read8(uint64* v) { return Read(v); }
bool BufferReader::Read8s(int64* v) { return Read(v); }

bool BufferReader::ReadFourCC(FourCC* v) {
  return Read4(reinterpret_cast<uint32*>(v));
}

bool BufferReader::ReadVec(std::vector<uint8>* vec, int count) {
  RCHECK(HasBytes(count));
  vec->clear();
  vec->insert(vec->end(), buf_ + pos_, buf_ + pos_ + count);
  pos_ += count;
  return true;
}

bool BufferReader::SkipBytes(int bytes) {
  RCHECK(HasBytes(bytes));
  pos_ += bytes;
  return true;
}

bool BufferReader::Read4Into8(uint64* v) {
  uint32 tmp;
  RCHECK(Read4(&tmp));
  *v = tmp;
  return true;
}

bool BufferReader::Read4sInto8s(int64* v) {
  // Beware of the need for sign extension.
  int32 tmp;
  RCHECK(Read4s(&tmp));
  *v = tmp;
  return true;
}


BoxReader::BoxReader(const uint8* buf, const int size,
                     const LogCB& log_cb)
    : BufferReader(buf, size),
      log_cb_(log_cb),
      type_(FOURCC_NULL),
      version_(0),
      flags_(0),
      scanned_(false) {
}

BoxReader::~BoxReader() {
  if (scanned_ && !children_.empty()) {
    for (ChildMap::iterator itr = children_.begin();
         itr != children_.end(); ++itr) {
      DVLOG(1) << "Skipping unknown box: " << FourCCToString(itr->first);
    }
  }
}

// static
BoxReader* BoxReader::ReadTopLevelBox(const uint8* buf,
                                      const int buf_size,
                                      const LogCB& log_cb,
                                      bool* err) {
  scoped_ptr<BoxReader> reader(new BoxReader(buf, buf_size, log_cb));
  if (!reader->ReadHeader(err))
    return NULL;

  if (!IsValidTopLevelBox(reader->type(), log_cb)) {
    *err = true;
    return NULL;
  }

  if (reader->size() <= buf_size)
    return reader.release();

  return NULL;
}

// static
bool BoxReader::StartTopLevelBox(const uint8* buf,
                                 const int buf_size,
                                 const LogCB& log_cb,
                                 FourCC* type,
                                 int* box_size,
                                 bool* err) {
  BoxReader reader(buf, buf_size, log_cb);
  if (!reader.ReadHeader(err)) return false;
  if (!IsValidTopLevelBox(reader.type(), log_cb)) {
    *err = true;
    return false;
  }
  *type = reader.type();
  *box_size = reader.size();
  return true;
}

// static
bool BoxReader::IsValidTopLevelBox(const FourCC& type,
                                   const LogCB& log_cb) {
  switch (type) {
    case FOURCC_FTYP:
    case FOURCC_PDIN:
    case FOURCC_BLOC:
    case FOURCC_MOOV:
    case FOURCC_MOOF:
    case FOURCC_MFRA:
    case FOURCC_MDAT:
    case FOURCC_FREE:
    case FOURCC_SKIP:
    case FOURCC_META:
    case FOURCC_MECO:
    case FOURCC_STYP:
    case FOURCC_SIDX:
    case FOURCC_SSIX:
    case FOURCC_PRFT:
    case FOURCC_UUID:
    case FOURCC_EMSG:
      return true;
    default:
      // Hex is used to show nonprintable characters and aid in debugging
      MEDIA_LOG(log_cb) << "Unrecognized top-level box type "
                        << FourCCToString(type);
      return false;
  }
}

bool BoxReader::ScanChildren() {
  DCHECK(!scanned_);
  scanned_ = true;

  bool err = false;
  while (pos() < size()) {
    BoxReader child(&buf_[pos_], size_ - pos_, log_cb_);
    if (!child.ReadHeader(&err)) break;

    children_.insert(std::pair<FourCC, BoxReader>(child.type(), child));
    pos_ += child.size();
  }

  DCHECK(!err);
  return !err && pos() == size();
}

bool BoxReader::HasChild(Box* child) {
  DCHECK(scanned_);
  DCHECK(child);
  return children_.count(child->BoxType()) > 0;
}

bool BoxReader::ReadChild(Box* child) {
  DCHECK(scanned_);
  FourCC child_type = child->BoxType();

  ChildMap::iterator itr = children_.find(child_type);
  RCHECK(itr != children_.end());
  DVLOG(2) << "Found a " << FourCCToString(child_type) << " box.";
  RCHECK(child->Parse(&itr->second));
  children_.erase(itr);
  return true;
}

bool BoxReader::MaybeReadChild(Box* child) {
  if (!children_.count(child->BoxType())) return true;
  return ReadChild(child);
}

bool BoxReader::ReadFullBoxHeader() {
  uint32 vflags;
  RCHECK(Read4(&vflags));
  version_ = vflags >> 24;
  flags_ = vflags & 0xffffff;
  return true;
}

bool BoxReader::ReadHeader(bool* err) {
  uint64 size = 0;
  *err = false;

  if (!HasBytes(8)) return false;
  CHECK(Read4Into8(&size) && ReadFourCC(&type_));

  if (size == 0) {
    // Media Source specific: we do not support boxes that run to EOS.
    *err = true;
    return false;
  } else if (size == 1) {
    if (!HasBytes(8)) return false;
    CHECK(Read8(&size));
  }

  // Implementation-specific: support for boxes larger than 2^31 has been
  // removed.
  if (size < static_cast<uint64>(pos_) ||
      size > static_cast<uint64>(kint32max)) {
    *err = true;
    return false;
  }

  // Note that the pos_ head has advanced to the byte immediately after the
  // header, which is where we want it.
  size_ = size;
  return true;
}

}  // namespace mp4
}  // namespace media
