// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_BROWSER_FILEAPI_FILE_OBSERVERS_H_
#define WEBKIT_BROWSER_FILEAPI_FILE_OBSERVERS_H_

#include "base/basictypes.h"
#include "webkit/browser/webkit_storage_browser_export.h"

// TODO(kinuko): Split this file into per-observer multiple files.

namespace fileapi {

class FileSystemURL;

// An abstract interface to observe update operations.
//
// OnStartUpdate and OnEndUpdate are called once for each target url
// before and after following operations regardless of whether the operation
// is made recursively or not (i.e. StartUpdate() will be called only once
// for destination url regardless of whether it is recursive copy or not):
//  CreateFile(), CreateDirectory(),
//  Copy() (destination only),
//  Move() (both for source and destination),
//  Remove(), Write(), Truncate(), TouchFile()
//
// OnUpdate() is called each time the |url| is updated but works only for
// sandboxed files (where usage is tracked).
class WEBKIT_STORAGE_BROWSER_EXPORT FileUpdateObserver {
 public:
  FileUpdateObserver() {}
  virtual ~FileUpdateObserver() {}

  virtual void OnStartUpdate(const FileSystemURL& url) = 0;
  virtual void OnUpdate(const FileSystemURL& url, int64 delta) = 0;
  virtual void OnEndUpdate(const FileSystemURL& url) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(FileUpdateObserver);
};

// An abstract interface to observe file access.
// OnAccess is called whenever an operation reads file contents or metadata.
// (It is called only once per operation regardless of whether the operation
// is recursive or not)
class WEBKIT_STORAGE_BROWSER_EXPORT FileAccessObserver {
 public:
  FileAccessObserver() {}
  virtual ~FileAccessObserver() {}

  virtual void OnAccess(const FileSystemURL& url) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(FileAccessObserver);
};

// An abstract interface to observe file changes.
// Each method of this class is called once per file/directory is created,
// removed or modified.  For recursive operations each method is called for
// each subdirectory/subfile.  Currently ChangeObserver is only supported
// by the local sandbox file system.
class WEBKIT_STORAGE_BROWSER_EXPORT FileChangeObserver {
 public:
  FileChangeObserver() {}
  virtual ~FileChangeObserver() {}

  virtual void OnCreateFile(const FileSystemURL& url) = 0;
  virtual void OnCreateFileFrom(const FileSystemURL& url,
                                const FileSystemURL& src) = 0;
  virtual void OnRemoveFile(const FileSystemURL& url) = 0;
  virtual void OnModifyFile(const FileSystemURL& url) = 0;

  virtual void OnCreateDirectory(const FileSystemURL& url) = 0;
  virtual void OnRemoveDirectory(const FileSystemURL& url) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(FileChangeObserver);
};

}  // namespace fileapi

#endif  // WEBKIT_BROWSER_FILEAPI_FILE_OBSERVERS_H_
