// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "vorg.h"

#include <vector>

// VORG - Vertical Origin Table
// http://www.microsoft.com/opentype/otspec/vorg.htm

#define DROP_THIS_TABLE \
  do { delete file->vorg; file->vorg = 0; } while (0)

namespace ots {

bool ots_vorg_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);
  file->vorg = new OpenTypeVORG;
  OpenTypeVORG * const vorg = file->vorg;

  uint16_t num_recs;
  if (!table.ReadU16(&vorg->major_version) ||
      !table.ReadU16(&vorg->minor_version) ||
      !table.ReadS16(&vorg->default_vert_origin_y) ||
      !table.ReadU16(&num_recs)) {
    return OTS_FAILURE();
  }
  if (vorg->major_version != 1) {
    OTS_WARNING("bad major version: %u", vorg->major_version);
    DROP_THIS_TABLE;
    return true;
  }
  if (vorg->minor_version != 0) {
    OTS_WARNING("bad minor version: %u", vorg->minor_version);
    DROP_THIS_TABLE;
    return true;
  }

  // num_recs might be zero (e.g., DFHSMinchoPro5-W3-Demo.otf).
  if (!num_recs) {
    return true;
  }

  uint16_t last_glyph_index = 0;
  vorg->metrics.reserve(num_recs);
  for (unsigned i = 0; i < num_recs; ++i) {
    OpenTypeVORGMetrics rec;

    if (!table.ReadU16(&rec.glyph_index) ||
        !table.ReadS16(&rec.vert_origin_y)) {
      return OTS_FAILURE();
    }
    if ((i != 0) && (rec.glyph_index <= last_glyph_index)) {
      OTS_WARNING("the table is not sorted");
      DROP_THIS_TABLE;
      return true;
    }
    last_glyph_index = rec.glyph_index;

    vorg->metrics.push_back(rec);
  }

  return true;
}

bool ots_vorg_should_serialise(OpenTypeFile *file) {
  if (!file->cff) return false;  // this table is not for fonts with TT glyphs.
  return file->vorg != NULL;
}

bool ots_vorg_serialise(OTSStream *out, OpenTypeFile *file) {
  OpenTypeVORG * const vorg = file->vorg;

  if (!out->WriteU16(vorg->major_version) ||
      !out->WriteU16(vorg->minor_version) ||
      !out->WriteS16(vorg->default_vert_origin_y) ||
      !out->WriteU16(vorg->metrics.size())) {
    return OTS_FAILURE();
  }

  for (unsigned i = 0; i < vorg->metrics.size(); ++i) {
    const OpenTypeVORGMetrics& rec = vorg->metrics[i];
    if (!out->WriteU16(rec.glyph_index) ||
        !out->WriteS16(rec.vert_origin_y)) {
      return OTS_FAILURE();
    }
  }

  return true;
}

void ots_vorg_free(OpenTypeFile *file) {
  delete file->vorg;
}

}  // namespace ots
