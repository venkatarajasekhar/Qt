// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "glyf.h"

#include <algorithm>
#include <limits>

#include "head.h"
#include "loca.h"
#include "maxp.h"

// glyf - Glyph Data
// http://www.microsoft.com/opentype/otspec/glyf.htm

namespace {

bool ParseFlagsForSimpleGlyph(ots::Buffer *table,
                              uint32_t gly_length,
                              uint32_t num_flags,
                              uint32_t *flags_count_logical,
                              uint32_t *flags_count_physical,
                              uint32_t *xy_coordinates_length) {
  uint8_t flag = 0;
  if (!table->ReadU8(&flag)) {
    return OTS_FAILURE();
  }

  uint32_t delta = 0;
  if (flag & (1u << 1)) {  // x-Short
    ++delta;
  } else if (!(flag & (1u << 4))) {
    delta += 2;
  }

  if (flag & (1u << 2)) {  // y-Short
    ++delta;
  } else if (!(flag & (1u << 5))) {
    delta += 2;
  }

  if (flag & (1u << 3)) {  // repeat
    if (*flags_count_logical + 1 >= num_flags) {
      return OTS_FAILURE();
    }
    uint8_t repeat = 0;
    if (!table->ReadU8(&repeat)) {
      return OTS_FAILURE();
    }
    if (repeat == 0) {
      return OTS_FAILURE();
    }
    delta += (delta * repeat);

    *flags_count_logical += repeat;
    if (*flags_count_logical >= num_flags) {
      return OTS_FAILURE();
    }
    ++(*flags_count_physical);
  }

  if ((flag & (1u << 6)) || (flag & (1u << 7))) {  // reserved flags
    return OTS_FAILURE();
  }

  *xy_coordinates_length += delta;
  if (gly_length < *xy_coordinates_length) {
    return OTS_FAILURE();
  }

  return true;
}

bool ParseSimpleGlyph(ots::OpenTypeFile *file, const uint8_t *data,
                      ots::Buffer *table, int16_t num_contours,
                      uint32_t gly_offset, uint32_t gly_length,
                      uint32_t *new_size) {
  ots::OpenTypeGLYF *glyf = file->glyf;

  // read the end-points array
  uint16_t num_flags = 0;
  for (int i = 0; i < num_contours; ++i) {
    uint16_t tmp_index = 0;
    if (!table->ReadU16(&tmp_index)) {
      return OTS_FAILURE();
    }
    if (tmp_index == 0xffffu) {
      return OTS_FAILURE();
    }
    // check if the indices are monotonically increasing
    if (i && (tmp_index + 1 <= num_flags)) {
      return OTS_FAILURE();
    }
    num_flags = tmp_index + 1;
  }

  uint16_t bytecode_length = 0;
  if (!table->ReadU16(&bytecode_length)) {
    return OTS_FAILURE();
  }
  if ((file->maxp->version_1) &&
      (file->maxp->max_size_glyf_instructions < bytecode_length)) {
    return OTS_FAILURE();
  }

  const uint32_t gly_header_length = 10 + num_contours * 2 + 2;
  if (gly_length < (gly_header_length + bytecode_length)) {
    return OTS_FAILURE();
  }

  if (ots::g_transcode_hints) {
    glyf->iov.push_back(std::make_pair(
        data + gly_offset,
        static_cast<size_t>(gly_header_length + bytecode_length)));
  } else {
    // enqueue two vectors: the glyph data up to the bytecode length, then
    // a pointer to a static uint16_t 0 to overwrite the length.
    glyf->iov.push_back(std::make_pair(
        data + gly_offset,
        static_cast<size_t>(gly_header_length - 2)));
    glyf->iov.push_back(std::make_pair((const uint8_t*) "\x00\x00",
                                       static_cast<size_t>(2)));
  }

  if (!table->Skip(bytecode_length)) {
    return OTS_FAILURE();
  }

  uint32_t flags_count_physical = 0;  // on memory
  uint32_t xy_coordinates_length = 0;
  for (uint32_t flags_count_logical = 0;
       flags_count_logical < num_flags;
       ++flags_count_logical, ++flags_count_physical) {
    if (!ParseFlagsForSimpleGlyph(table,
                                  gly_length,
                                  num_flags,
                                  &flags_count_logical,
                                  &flags_count_physical,
                                  &xy_coordinates_length)) {
      return OTS_FAILURE();
    }
  }

  if (gly_length < (gly_header_length + bytecode_length +
                    flags_count_physical + xy_coordinates_length)) {
    return OTS_FAILURE();
  }

  if (gly_length - (gly_header_length + bytecode_length +
                    flags_count_physical + xy_coordinates_length) > 3) {
    // We allow 0-3 bytes difference since gly_length is 4-bytes aligned,
    // zero-padded length.
    return OTS_FAILURE();
  }

  glyf->iov.push_back(std::make_pair(
      data + gly_offset + gly_header_length + bytecode_length,
      static_cast<size_t>(flags_count_physical + xy_coordinates_length)));

  *new_size
      = gly_header_length + flags_count_physical + xy_coordinates_length;
  if (ots::g_transcode_hints) {
    *new_size += bytecode_length;
  }

  return true;
}

}  // namespace

namespace ots {

bool ots_glyf_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);

  if (!file->maxp || !file->loca || !file->head) {
    return OTS_FAILURE();
  }

  OpenTypeGLYF *glyf = new OpenTypeGLYF;
  file->glyf = glyf;

  const unsigned num_glyphs = file->maxp->num_glyphs;
  std::vector<uint32_t> &offsets = file->loca->offsets;

  if (offsets.size() != num_glyphs + 1) {
    return OTS_FAILURE();
  }

  std::vector<uint32_t> resulting_offsets(num_glyphs + 1);
  uint32_t current_offset = 0;

  for (unsigned i = 0; i < num_glyphs; ++i) {
    const unsigned gly_offset = offsets[i];
    // The LOCA parser checks that these values are monotonic
    const unsigned gly_length = offsets[i + 1] - offsets[i];
    if (!gly_length) {
      // this glyph has no outline (e.g. the space charactor)
      resulting_offsets[i] = current_offset;
      continue;
    }

    if (gly_offset >= length) {
      return OTS_FAILURE();
    }
    // Since these are unsigned types, the compiler is not allowed to assume
    // that they never overflow.
    if (gly_offset + gly_length < gly_offset) {
      return OTS_FAILURE();
    }
    if (gly_offset + gly_length > length) {
      return OTS_FAILURE();
    }

    table.set_offset(gly_offset);
    int16_t num_contours, xmin, ymin, xmax, ymax;
    if (!table.ReadS16(&num_contours) ||
        !table.ReadS16(&xmin) ||
        !table.ReadS16(&ymin) ||
        !table.ReadS16(&xmax) ||
        !table.ReadS16(&ymax)) {
      return OTS_FAILURE();
    }

    if (num_contours <= -2) {
      // -2, -3, -4, ... are reserved for future use.
      return OTS_FAILURE();
    }

    // workaround for fonts in http://www.princexml.com/fonts/
    if ((xmin == 32767) &&
        (xmax == -32767) &&
        (ymin == 32767) &&
        (ymax == -32767)) {
      OTS_WARNING("bad xmin/xmax/ymin/ymax values");
      xmin = xmax = ymin = ymax = 0;
    }

    if (xmin > xmax || ymin > ymax) {
      return OTS_FAILURE();
    }

    unsigned new_size = 0;
    if (num_contours >= 0) {
      // this is a simple glyph and might contain bytecode
      if (!ParseSimpleGlyph(file, data, &table,
                            num_contours, gly_offset, gly_length, &new_size)) {
        return OTS_FAILURE();
      }
    } else {
      // it's a composite glyph without any bytecode. Enqueue the whole thing
      glyf->iov.push_back(std::make_pair(data + gly_offset,
                                         static_cast<size_t>(gly_length)));
      new_size = gly_length;
    }

    resulting_offsets[i] = current_offset;
    // glyphs must be four byte aligned
    // TODO(yusukes): investigate whether this padding is really necessary.
    //                Which part of the spec requires this?
    const unsigned padding = (4 - (new_size & 3)) % 4;
    if (padding) {
      glyf->iov.push_back(std::make_pair(
          reinterpret_cast<const uint8_t*>("\x00\x00\x00\x00"),
          static_cast<size_t>(padding)));
      new_size += padding;
    }
    current_offset += new_size;
  }
  resulting_offsets[num_glyphs] = current_offset;

  const uint16_t max16 = std::numeric_limits<uint16_t>::max();
  if ((*std::max_element(resulting_offsets.begin(),
                         resulting_offsets.end()) >= (max16 * 2u)) &&
      (file->head->index_to_loc_format != 1)) {
    OTS_WARNING("2-bytes indexing is not possible (due to the padding above)");
    file->head->index_to_loc_format = 1;
  }

  file->loca->offsets = resulting_offsets;
  return true;
}

bool ots_glyf_should_serialise(OpenTypeFile *file) {
  return file->glyf != NULL;
}

bool ots_glyf_serialise(OTSStream *out, OpenTypeFile *file) {
  const OpenTypeGLYF *glyf = file->glyf;

  for (unsigned i = 0; i < glyf->iov.size(); ++i) {
    if (!out->Write(glyf->iov[i].first, glyf->iov[i].second)) {
      return OTS_FAILURE();
    }
  }

  return true;
}

void ots_glyf_free(OpenTypeFile *file) {
  delete file->glyf;
}

}  // namespace ots
