// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/browser/fileapi/sandbox_origin_database_interface.h"

namespace fileapi {

SandboxOriginDatabaseInterface::OriginRecord::OriginRecord() {
}

SandboxOriginDatabaseInterface::OriginRecord::OriginRecord(
    const std::string& origin_in, const base::FilePath& path_in)
    : origin(origin_in), path(path_in) {
}

SandboxOriginDatabaseInterface::OriginRecord::~OriginRecord() {
}

}  // namespace fileapi
