// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_SHELL_DIALOGS_SELECTED_FILE_INFO_H_
#define UI_SHELL_DIALOGS_SELECTED_FILE_INFO_H_

#include <vector>

#include "base/files/file_path.h"
#include "base/strings/string16.h"
#include "ui/shell_dialogs/shell_dialogs_export.h"

namespace ui {

// Struct used for passing selected file info to WebKit.
struct SHELL_DIALOGS_EXPORT SelectedFileInfo {
  // Selected file's user friendly path as seen in the UI.
  base::FilePath file_path;

  // The actual local path to the selected file. This can be a snapshot file
  // with a human unreadable name like /blah/.d41d8cd98f00b204e9800998ecf8427e.
  // |real_path| can differ from |file_path| for drive files (e.g.
  // /drive_cache/temporary/d41d8cd98f00b204e9800998ecf8427e vs.
  // /special/drive/foo.txt).
  // If not set, defaults to |file_path|.
  base::FilePath local_path;

  // This field is optional. The display name contains only the base name
  // portion of a file name (ex. no path separators), and used for displaying
  // selected file names. If this field is empty, the base name portion of
  // |path| is used for displaying.
  base::FilePath::StringType display_name;

  SelectedFileInfo();
  SelectedFileInfo(const base::FilePath& in_file_path,
                   const base::FilePath& in_local_path);
  ~SelectedFileInfo();
};

}  // namespace ui

#endif  // UI_SHELL_DIALOGS_SELECTED_FILE_INFO_H_

