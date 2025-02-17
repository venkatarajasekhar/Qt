// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_BROWSER_FILEAPI_MOCK_FILE_CHANGE_OBSERVER_H_
#define WEBKIT_BROWSER_FILEAPI_MOCK_FILE_CHANGE_OBSERVER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "webkit/browser/fileapi/file_observers.h"
#include "webkit/browser/fileapi/file_system_url.h"
#include "webkit/browser/fileapi/task_runner_bound_observer_list.h"

namespace fileapi {

// Mock file change observer.
class MockFileChangeObserver : public FileChangeObserver {
 public:
  MockFileChangeObserver();
  virtual ~MockFileChangeObserver();

  // Creates a ChangeObserverList which only contains given |observer|.
  static ChangeObserverList CreateList(MockFileChangeObserver* observer);

  // FileChangeObserver overrides.
  virtual void OnCreateFile(const FileSystemURL& url) OVERRIDE;
  virtual void OnCreateFileFrom(const FileSystemURL& url,
                                const FileSystemURL& src) OVERRIDE;
  virtual void OnRemoveFile(const FileSystemURL& url) OVERRIDE;
  virtual void OnModifyFile(const FileSystemURL& url) OVERRIDE;
  virtual void OnCreateDirectory(const FileSystemURL& url) OVERRIDE;
  virtual void OnRemoveDirectory(const FileSystemURL& url) OVERRIDE;

  void ResetCount() {
    create_file_count_ = 0;
    create_file_from_count_ = 0;
    remove_file_count_ = 0;
    modify_file_count_ = 0;
    create_directory_count_ = 0;
    remove_directory_count_ = 0;
  }

  bool HasNoChange() const {
    return create_file_count_ == 0 &&
           create_file_from_count_ == 0 &&
           remove_file_count_ == 0 &&
           modify_file_count_ == 0 &&
           create_directory_count_ == 0 &&
           remove_directory_count_ == 0;
  }

  int create_file_count() const { return create_file_count_; }
  int create_file_from_count() const { return create_file_from_count_; }
  int remove_file_count() const { return remove_file_count_; }
  int modify_file_count() const { return modify_file_count_; }
  int create_directory_count() const { return create_directory_count_; }
  int remove_directory_count() const { return remove_directory_count_; }

  int get_and_reset_create_file_count() {
    int tmp = create_file_count_;
    create_file_count_ = 0;
    return tmp;
  }
  int get_and_reset_create_file_from_count() {
    int tmp = create_file_from_count_;
    create_file_from_count_ = 0;
    return tmp;
  }
  int get_and_reset_remove_file_count() {
    int tmp = remove_file_count_;
    remove_file_count_ = 0;
    return tmp;
  }
  int get_and_reset_modify_file_count() {
    int tmp = modify_file_count_;
    modify_file_count_ = 0;
    return tmp;
  }
  int get_and_reset_create_directory_count() {
    int tmp = create_directory_count_;
    create_directory_count_ = 0;
    return tmp;
  }
  int get_and_reset_remove_directory_count() {
    int tmp = remove_directory_count_;
    remove_directory_count_ = 0;
    return tmp;
  }

 private:
  int create_file_count_;
  int create_file_from_count_;
  int remove_file_count_;
  int modify_file_count_;
  int create_directory_count_;
  int remove_directory_count_;

  DISALLOW_COPY_AND_ASSIGN(MockFileChangeObserver);
};

}  // namespace fileapi

#endif  // WEBKIT_BROWSER_FILEAPI_MOCK_FILE_CHANGE_OBSERVER_H_
