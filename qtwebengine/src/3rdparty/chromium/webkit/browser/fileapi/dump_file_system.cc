// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A tool to dump HTML5 filesystem from CUI.
//
// Usage:
//
// ./out/Release/dump_file_system [options] <filesystem dir> [origin]...
//
// If no origin is specified, this dumps all origins in the profile dir.
//
// Available options:
//
// -t : dumps temporary files instead of persistent.
// -s : dumps syncable files instead of persistent.
// -l : more information will be displayed.
//
// The format of -l option is:
//
// === ORIGIN origin_name origin_dir ===
// file_name file_id file_size file_content_path
// ...
//
// where file_name has a trailing slash, file_size is the number of
// children, and file_content_path is empty if the file is a directory.
//

#include <stdio.h>
#include <stdlib.h>

#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/format_macros.h"
#include "base/strings/stringprintf.h"
#include "webkit/browser/fileapi/obfuscated_file_util.h"
#include "webkit/browser/fileapi/sandbox_directory_database.h"
#include "webkit/browser/fileapi/sandbox_file_system_backend.h"
#include "webkit/browser/fileapi/sandbox_origin_database.h"
#include "webkit/common/fileapi/file_system_types.h"
#include "webkit/common/fileapi/file_system_util.h"

namespace {

bool g_opt_long;
const char* g_opt_fs_type = "p";

void ShowMessageAndExit(const std::string& msg) {
  fprintf(stderr, "%s\n", msg.c_str());
  exit(EXIT_FAILURE);
}

void ShowUsageAndExit(const std::string& arg0) {
  ShowMessageAndExit(
      "Usage: " + arg0 +
      " [-l] [-t] [-s] <filesystem dir> [origin]...");
}

}  // namespace

namespace fileapi {

static void DumpDirectoryTree(const std::string& origin_name,
                              base::FilePath origin_dir) {
  origin_dir = origin_dir.Append(g_opt_fs_type);

  printf("=== ORIGIN %s %s ===\n",
         origin_name.c_str(), FilePathToString(origin_dir).c_str());

  if (!base::DirectoryExists(origin_dir))
    return;

  SandboxDirectoryDatabase directory_db(origin_dir, NULL);
  SandboxDirectoryDatabase::FileId root_id;
  if (!directory_db.GetFileWithPath(StringToFilePath("/"), &root_id))
    return;

  std::stack<std::pair<SandboxDirectoryDatabase::FileId,
                       std::string> > paths;
  paths.push(std::make_pair(root_id, ""));
  while (!paths.empty()) {
    SandboxDirectoryDatabase::FileId id = paths.top().first;
    const std::string dirname = paths.top().second;
    paths.pop();

    SandboxDirectoryDatabase::FileInfo info;
    if (!directory_db.GetFileInfo(id, &info)) {
      ShowMessageAndExit(base::StringPrintf("GetFileInfo failed for %"PRId64,
                                            id));
    }

    const std::string name =
        dirname + "/" + FilePathToString(base::FilePath(info.name));
    std::vector<SandboxDirectoryDatabase::FileId> children;
    if (info.is_directory()) {
      if (!directory_db.ListChildren(id, &children)) {
        ShowMessageAndExit(base::StringPrintf(
            "ListChildren failed for %s (%"PRId64")",
            info.name.c_str(), id));
      }

      for (size_t j = children.size(); j; j--)
        paths.push(make_pair(children[j-1], name));
    }

    // +1 for the leading extra slash.
    const char* display_name = name.c_str() + 1;
    const char* directory_suffix = info.is_directory() ? "/" : "";
    if (g_opt_long) {
      int64 size;
      if (info.is_directory()) {
        size = static_cast<int64>(children.size());
      } else {
        base::GetFileSize(origin_dir.Append(info.data_path), &size);
      }
      // TODO(hamaji): Modification time?
      printf("%s%s %"PRId64" %"PRId64" %s\n",
             display_name,
             directory_suffix,
             id,
             size,
             FilePathToString(info.data_path).c_str());
    } else {
      printf("%s%s\n", display_name, directory_suffix);
    }
  }
}

static void DumpOrigin(const base::FilePath& file_system_dir,
                       const std::string& origin_name) {
  SandboxOriginDatabase origin_db(file_system_dir, NULL);
  base::FilePath origin_dir;
  if (!origin_db.HasOriginPath(origin_name)) {
    ShowMessageAndExit("Origin " + origin_name + " is not in " +
                       FilePathToString(file_system_dir));
  }

  if (!origin_db.GetPathForOrigin(origin_name, &origin_dir)) {
    ShowMessageAndExit("Failed to get path of origin " + origin_name +
                       " in " + FilePathToString(file_system_dir));
  }
  DumpDirectoryTree(origin_name, file_system_dir.Append(origin_dir));
}

static void DumpFileSystem(const base::FilePath& file_system_dir) {
  SandboxOriginDatabase origin_db(file_system_dir, NULL);
  std::vector<SandboxOriginDatabase::OriginRecord> origins;
  origin_db.ListAllOrigins(&origins);
  for (size_t i = 0; i < origins.size(); i++) {
    const SandboxOriginDatabase::OriginRecord& origin = origins[i];
    DumpDirectoryTree(origin.origin, file_system_dir.Append(origin.path));
    puts("");
  }
}

}  // namespace fileapi

int main(int argc, char* argv[]) {
  const char* arg0 = argv[0];
  std::string username = "Default";
  while (true) {
    if (argc < 2)
      ShowUsageAndExit(arg0);

    if (std::string(argv[1]) == "-l") {
      g_opt_long = true;
      argc--;
      argv++;
    } else if (std::string(argv[1]) == "-t") {
      g_opt_fs_type = "t";
      argc--;
      argv++;
    } else if (std::string(argv[1]) == "-s") {
      g_opt_fs_type = "s";
      argc--;
      argv++;
    } else {
      break;
    }
  }

  if (argc < 2)
    ShowUsageAndExit(arg0);

  const base::FilePath file_system_dir = fileapi::StringToFilePath(argv[1]);
  if (!base::DirectoryExists(file_system_dir)) {
    ShowMessageAndExit(fileapi::FilePathToString(file_system_dir) +
                       " is not a filesystem directory");
  }

  if (argc == 2) {
    fileapi::DumpFileSystem(file_system_dir);
  } else {
    for (int i = 2; i < argc; i++) {
      fileapi::DumpOrigin(file_system_dir, argv[i]);
    }
  }
  return 0;
}
