// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_value.h"

#include "base/logging.h"

namespace content {

IndexedDBValue::IndexedDBValue() {}
IndexedDBValue::IndexedDBValue(
    const std::string& input_bits,
    const std::vector<IndexedDBBlobInfo>& input_blob_info)
    : bits(input_bits), blob_info(input_blob_info) {
  DCHECK(!input_blob_info.size() || input_bits.size());
}
IndexedDBValue::~IndexedDBValue() {}

}  // namespace content
