// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/browser/fileapi/native_file_util.h"

#include "base/file_util.h"
#include "base/files/file.h"
#include "base/files/file_enumerator.h"
#include "base/memory/scoped_ptr.h"
#include "webkit/browser/fileapi/file_system_operation_context.h"
#include "webkit/browser/fileapi/file_system_url.h"

namespace fileapi {

namespace {

// Sets permissions on directory at |dir_path| based on the target platform.
// Returns true on success, or false otherwise.
//
// TODO(benchan): Find a better place outside webkit to host this function.
bool SetPlatformSpecificDirectoryPermissions(const base::FilePath& dir_path) {
#if defined(OS_CHROMEOS)
    // System daemons on Chrome OS may run as a user different than the Chrome
    // process but need to access files under the directories created here.
    // Because of that, grant the execute permission on the created directory
    // to group and other users.
    if (HANDLE_EINTR(chmod(dir_path.value().c_str(),
                           S_IRWXU | S_IXGRP | S_IXOTH)) != 0) {
      return false;
    }
#endif
    // Keep the directory permissions unchanged on non-Chrome OS platforms.
    return true;
}

// Copies a file |from| to |to|, and ensure the written content is synced to
// the disk. This is essentially base::CopyFile followed by fsync().
bool CopyFileAndSync(const base::FilePath& from, const base::FilePath& to) {
  base::File infile(from, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!infile.IsValid()) {
    return false;
  }

  base::File outfile(to,
                     base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!outfile.IsValid()) {
    return false;
  }

  const int kBufferSize = 32768;
  std::vector<char> buffer(kBufferSize);

  for (;;) {
    int bytes_read = infile.ReadAtCurrentPos(&buffer[0], kBufferSize);
    if (bytes_read < 0)
      return false;
    if (bytes_read == 0)
      break;
    for (int bytes_written = 0; bytes_written < bytes_read; ) {
      int bytes_written_partial = outfile.WriteAtCurrentPos(
          &buffer[bytes_written], bytes_read - bytes_written);
      if (bytes_written_partial < 0)
        return false;
      bytes_written += bytes_written_partial;
    }
  }

  return outfile.Flush();
}

}  // namespace

using base::PlatformFile;

class NativeFileEnumerator : public FileSystemFileUtil::AbstractFileEnumerator {
 public:
  NativeFileEnumerator(const base::FilePath& root_path,
                       bool recursive,
                       int file_type)
    : file_enum_(root_path, recursive, file_type) {
  }

  virtual ~NativeFileEnumerator() {}

  virtual base::FilePath Next() OVERRIDE;
  virtual int64 Size() OVERRIDE;
  virtual base::Time LastModifiedTime() OVERRIDE;
  virtual bool IsDirectory() OVERRIDE;

 private:
  base::FileEnumerator file_enum_;
  base::FileEnumerator::FileInfo file_util_info_;
};

base::FilePath NativeFileEnumerator::Next() {
  base::FilePath rv = file_enum_.Next();
  if (!rv.empty())
    file_util_info_ = file_enum_.GetInfo();
  return rv;
}

int64 NativeFileEnumerator::Size() {
  return file_util_info_.GetSize();
}

base::Time NativeFileEnumerator::LastModifiedTime() {
  return file_util_info_.GetLastModifiedTime();
}

bool NativeFileEnumerator::IsDirectory() {
  return file_util_info_.IsDirectory();
}

NativeFileUtil::CopyOrMoveMode NativeFileUtil::CopyOrMoveModeForDestination(
    const FileSystemURL& dest_url, bool copy) {
  if (copy) {
    return dest_url.mount_option().copy_sync_option() == COPY_SYNC_OPTION_SYNC ?
        COPY_SYNC : COPY_NOSYNC;
  }
  return MOVE;
}

base::File NativeFileUtil::CreateOrOpen(const base::FilePath& path,
                                        int file_flags) {
  if (!base::DirectoryExists(path.DirName())) {
    // If its parent does not exist, should return NOT_FOUND error.
    return base::File(base::File::FILE_ERROR_NOT_FOUND);
  }

  // TODO(rvargas): Check |file_flags| instead. See bug 356358.
  if (base::DirectoryExists(path))
    return base::File(base::File::FILE_ERROR_NOT_A_FILE);

  return base::File(path, file_flags);
}

base::File::Error NativeFileUtil::EnsureFileExists(
    const base::FilePath& path,
    bool* created) {
  if (!base::DirectoryExists(path.DirName()))
    // If its parent does not exist, should return NOT_FOUND error.
    return base::File::FILE_ERROR_NOT_FOUND;

  // Tries to create the |path| exclusively.  This should fail
  // with base::File::FILE_ERROR_EXISTS if the path already exists.
  base::File file(path, base::File::FLAG_CREATE | base::File::FLAG_READ);

  if (file.IsValid()) {
    if (created)
      *created = file.created();
    return base::File::FILE_OK;
  }

  base::File::Error error_code = file.error_details();
  if (error_code == base::File::FILE_ERROR_EXISTS) {
    // Make sure created_ is false.
    if (created)
      *created = false;
    error_code = base::File::FILE_OK;
  }
  return error_code;
}

base::File::Error NativeFileUtil::CreateDirectory(
    const base::FilePath& path,
    bool exclusive,
    bool recursive) {
  // If parent dir of file doesn't exist.
  if (!recursive && !base::PathExists(path.DirName()))
    return base::File::FILE_ERROR_NOT_FOUND;

  bool path_exists = base::PathExists(path);
  if (exclusive && path_exists)
    return base::File::FILE_ERROR_EXISTS;

  // If file exists at the path.
  if (path_exists && !base::DirectoryExists(path))
    return base::File::FILE_ERROR_EXISTS;

  if (!base::CreateDirectory(path))
    return base::File::FILE_ERROR_FAILED;

  if (!SetPlatformSpecificDirectoryPermissions(path)) {
    // Since some file systems don't support permission setting, we do not treat
    // an error from the function as the failure of copying. Just log it.
    LOG(WARNING) << "Setting directory permission failed: "
        << path.AsUTF8Unsafe();
  }

  return base::File::FILE_OK;
}

base::File::Error NativeFileUtil::GetFileInfo(
    const base::FilePath& path,
    base::File::Info* file_info) {
  if (!base::PathExists(path))
    return base::File::FILE_ERROR_NOT_FOUND;

  if (!base::GetFileInfo(path, file_info))
    return base::File::FILE_ERROR_FAILED;
  return base::File::FILE_OK;
}

scoped_ptr<FileSystemFileUtil::AbstractFileEnumerator>
    NativeFileUtil::CreateFileEnumerator(const base::FilePath& root_path,
                                         bool recursive) {
  return make_scoped_ptr(new NativeFileEnumerator(
      root_path, recursive,
      base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES))
      .PassAs<FileSystemFileUtil::AbstractFileEnumerator>();
}

base::File::Error NativeFileUtil::Touch(
    const base::FilePath& path,
    const base::Time& last_access_time,
    const base::Time& last_modified_time) {
  if (!base::TouchFile(path, last_access_time, last_modified_time))
    return base::File::FILE_ERROR_FAILED;
  return base::File::FILE_OK;
}

base::File::Error NativeFileUtil::Truncate(const base::FilePath& path,
                                           int64 length) {
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_WRITE);
  if (!file.IsValid())
    return file.error_details();

  if (!file.SetLength(length))
    return base::File::FILE_ERROR_FAILED;

  return base::File::FILE_OK;
}

bool NativeFileUtil::PathExists(const base::FilePath& path) {
  return base::PathExists(path);
}

bool NativeFileUtil::DirectoryExists(const base::FilePath& path) {
  return base::DirectoryExists(path);
}

base::File::Error NativeFileUtil::CopyOrMoveFile(
    const base::FilePath& src_path,
    const base::FilePath& dest_path,
    FileSystemOperation::CopyOrMoveOption option,
    CopyOrMoveMode mode) {
  base::File::Info info;
  base::File::Error error = NativeFileUtil::GetFileInfo(src_path, &info);
  if (error != base::File::FILE_OK)
    return error;
  if (info.is_directory)
    return base::File::FILE_ERROR_NOT_A_FILE;
  base::Time last_modified = info.last_modified;

  error = NativeFileUtil::GetFileInfo(dest_path, &info);
  if (error != base::File::FILE_OK &&
      error != base::File::FILE_ERROR_NOT_FOUND)
    return error;
  if (info.is_directory)
    return base::File::FILE_ERROR_INVALID_OPERATION;
  if (error == base::File::FILE_ERROR_NOT_FOUND) {
    error = NativeFileUtil::GetFileInfo(dest_path.DirName(), &info);
    if (error != base::File::FILE_OK)
      return error;
    if (!info.is_directory)
      return base::File::FILE_ERROR_NOT_FOUND;
  }

  switch (mode) {
    case COPY_NOSYNC:
      if (!base::CopyFile(src_path, dest_path))
        return base::File::FILE_ERROR_FAILED;
      break;
    case COPY_SYNC:
      if (!CopyFileAndSync(src_path, dest_path))
        return base::File::FILE_ERROR_FAILED;
      break;
    case MOVE:
      if (!base::Move(src_path, dest_path))
        return base::File::FILE_ERROR_FAILED;
      break;
  }

  // Preserve the last modified time. Do not return error here even if
  // the setting is failed, because the copy itself is successfully done.
  if (option == FileSystemOperation::OPTION_PRESERVE_LAST_MODIFIED)
    base::TouchFile(dest_path, last_modified, last_modified);

  return base::File::FILE_OK;
}

base::File::Error NativeFileUtil::DeleteFile(const base::FilePath& path) {
  if (!base::PathExists(path))
    return base::File::FILE_ERROR_NOT_FOUND;
  if (base::DirectoryExists(path))
    return base::File::FILE_ERROR_NOT_A_FILE;
  if (!base::DeleteFile(path, false))
    return base::File::FILE_ERROR_FAILED;
  return base::File::FILE_OK;
}

base::File::Error NativeFileUtil::DeleteDirectory(const base::FilePath& path) {
  if (!base::PathExists(path))
    return base::File::FILE_ERROR_NOT_FOUND;
  if (!base::DirectoryExists(path))
    return base::File::FILE_ERROR_NOT_A_DIRECTORY;
  if (!base::IsDirectoryEmpty(path))
    return base::File::FILE_ERROR_NOT_EMPTY;
  if (!base::DeleteFile(path, false))
    return base::File::FILE_ERROR_FAILED;
  return base::File::FILE_OK;
}

}  // namespace fileapi
