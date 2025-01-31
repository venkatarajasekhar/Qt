// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hdmx.h"
#include "head.h"
#include "maxp.h"

// hdmx - Horizontal Device Metrics
// http://www.microsoft.com/opentype/otspec/hdmx.htm

#define DROP_THIS_TABLE \
  do { delete file->hdmx; file->hdmx = 0; } while (0)

namespace ots {

bool ots_hdmx_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);
  file->hdmx = new OpenTypeHDMX;
  OpenTypeHDMX * const hdmx = file->hdmx;

  if (!file->head || !file->maxp) {
    return OTS_FAILURE();
  }

  if ((file->head->flags & 0x14) == 0) {
    // http://www.microsoft.com/typography/otspec/recom.htm
    OTS_WARNING("the table should not be present when bit 2 and 4 of the "
                "head->flags are not set");
    DROP_THIS_TABLE;
    return true;
  }

  int16_t num_recs;
  if (!table.ReadU16(&hdmx->version) ||
      !table.ReadS16(&num_recs) ||
      !table.ReadS32(&hdmx->size_device_record)) {
    return OTS_FAILURE();
  }
  if (hdmx->version != 0) {
    OTS_WARNING("bad version: %u", hdmx->version);
    DROP_THIS_TABLE;
    return true;
  }
  if (num_recs <= 0) {
    OTS_WARNING("bad num_recs: %d", num_recs);
    DROP_THIS_TABLE;
    return true;
  }
  const int32_t actual_size_device_record = file->maxp->num_glyphs + 2;
  if (hdmx->size_device_record < actual_size_device_record) {
    OTS_WARNING("bad hdmx->size_device_record: %d", hdmx->size_device_record);
    DROP_THIS_TABLE;
    return true;
  }

  hdmx->pad_len = hdmx->size_device_record - actual_size_device_record;
  if (hdmx->pad_len > 3) {
    return OTS_FAILURE();
  }

  uint8_t last_pixel_size = 0;
  hdmx->records.reserve(num_recs);
  for (int i = 0; i < num_recs; ++i) {
    OpenTypeHDMXDeviceRecord rec;

    if (!table.ReadU8(&rec.pixel_size) ||
        !table.ReadU8(&rec.max_width)) {
      return OTS_FAILURE();
    }
    if ((i != 0) &&
        (rec.pixel_size <= last_pixel_size)) {
      OTS_WARNING("records are not sorted");
      DROP_THIS_TABLE;
      return true;
    }
    last_pixel_size = rec.pixel_size;

    rec.widths.reserve(file->maxp->num_glyphs);
    for (unsigned j = 0; j < file->maxp->num_glyphs; ++j) {
      uint8_t width;
      if (!table.ReadU8(&width)) {
        return OTS_FAILURE();
      }
      rec.widths.push_back(width);
    }

    if ((hdmx->pad_len > 0) &&
        !table.Skip(hdmx->pad_len)) {
      return OTS_FAILURE();
    }

    hdmx->records.push_back(rec);
  }

  return true;
}

bool ots_hdmx_should_serialise(OpenTypeFile *file) {
  if (!file->hdmx) return false;
  if (!file->glyf) return false;  // this table is not for CFF fonts.
  return true;
}

bool ots_hdmx_serialise(OTSStream *out, OpenTypeFile *file) {
  OpenTypeHDMX * const hdmx = file->hdmx;

  if (!out->WriteU16(hdmx->version) ||
      !out->WriteS16(hdmx->records.size()) ||
      !out->WriteS32(hdmx->size_device_record)) {
    return OTS_FAILURE();
  }

  for (unsigned i = 0; i < hdmx->records.size(); ++i) {
    const OpenTypeHDMXDeviceRecord& rec = hdmx->records[i];
    if (!out->Write(&rec.pixel_size, 1) ||
        !out->Write(&rec.max_width, 1) ||
        !out->Write(&rec.widths[0], rec.widths.size())) {
      return OTS_FAILURE();
    }
    if ((hdmx->pad_len > 0) &&
        !out->Write((const uint8_t *)"\x00\x00\x00", hdmx->pad_len)) {
      return OTS_FAILURE();
    }
  }

  return true;
}

void ots_hdmx_free(OpenTypeFile *file) {
  delete file->hdmx;
}

}  // namespace ots
