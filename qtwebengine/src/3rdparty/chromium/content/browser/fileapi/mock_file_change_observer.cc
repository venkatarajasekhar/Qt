// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/fileapi/mock_file_change_observer.h"

namespace fileapi {

MockFileChangeObserver::MockFileChangeObserver()
    : create_file_count_(0),
      create_file_from_count_(0),
      remove_file_count_(0),
      modify_file_count_(0),
      create_directory_count_(0),
      remove_directory_count_(0) {}

MockFileChangeObserver::~MockFileChangeObserver() {}

// static
ChangeObserverList MockFileChangeObserver::CreateList(
    MockFileChangeObserver* observer) {
  ChangeObserverList list;
  return list.AddObserver(observer, base::MessageLoopProxy::current().get());
}

void MockFileChangeObserver::OnCreateFile(const FileSystemURL& url) {
  create_file_count_++;
}

void MockFileChangeObserver::OnCreateFileFrom(const FileSystemURL& url,
                                              const FileSystemURL& src) {
  create_file_from_count_++;
}

void MockFileChangeObserver::OnRemoveFile(const FileSystemURL& url) {
  remove_file_count_++;
}

void MockFileChangeObserver::OnModifyFile(const FileSystemURL& url) {
  modify_file_count_++;
}

void MockFileChangeObserver::OnCreateDirectory(const FileSystemURL& url) {
  create_directory_count_++;
}

void MockFileChangeObserver::OnRemoveDirectory(const FileSystemURL& url) {
  remove_directory_count_++;
}

}  // namespace fileapi
