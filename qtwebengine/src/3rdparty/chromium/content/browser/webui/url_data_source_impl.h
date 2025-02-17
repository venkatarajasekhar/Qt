// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBUI_URL_DATA_SOURCE_IMPL_H_
#define CONTENT_BROWSER_WEBUI_URL_DATA_SOURCE_IMPL_H_

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/sequenced_task_runner_helpers.h"
#include "content/browser/webui/url_data_manager.h"
#include "content/common/content_export.h"

namespace base {
class RefCountedMemory;
}

namespace content {
class URLDataManagerBackend;
class URLDataSource;
class URLDataSourceImpl;

// Trait used to handle deleting a URLDataSource. Deletion happens on the UI
// thread.
//
// Implementation note: the normal shutdown sequence is for the UI loop to
// stop pumping events then the IO loop and thread are stopped. When the
// URLDataSources are no longer referenced (which happens when IO thread stops)
// they get added to the UI message loop for deletion. But because the UI loop
// has stopped by the time this happens the URLDataSources would be leaked.
//
// To make sure URLDataSources are properly deleted URLDataManager manages
// deletion of the URLDataSources.  When a URLDataSource is no longer referenced
// it is added to |data_sources_| and a task is posted to the UI thread to
// handle the actual deletion. During shutdown |DeleteDataSources| is invoked so
// that all pending URLDataSources are properly deleted.
struct DeleteURLDataSource {
  static void Destruct(const URLDataSourceImpl* data_source) {
    URLDataManager::DeleteDataSource(data_source);
  }
};

// A URLDataSource is an object that can answer requests for data
// asynchronously. URLDataSources are collectively owned with refcounting smart
// pointers and should never be deleted on the IO thread, since their calls
// are handled almost always on the UI thread and there's a possibility of a
// data race.  The |DeleteDataSource| trait above is used to enforce this.
class URLDataSourceImpl : public base::RefCountedThreadSafe<
    URLDataSourceImpl, DeleteURLDataSource> {
 public:
  // See source_name_ below for docs on that parameter. Takes ownership of
  // |source|.
  URLDataSourceImpl(const std::string& source_name,
                    URLDataSource* source);

  // Report that a request has resulted in the data |bytes|.
  // If the request can't be satisfied, pass NULL for |bytes| to indicate
  // the request is over.
  virtual void SendResponse(int request_id, base::RefCountedMemory* bytes);

  const std::string& source_name() const { return source_name_; }
  URLDataSource* source() const { return source_.get(); }

 protected:
  virtual ~URLDataSourceImpl();

 private:
  friend class URLDataManager;
  friend class URLDataManagerBackend;
  friend class base::DeleteHelper<URLDataSourceImpl>;

  // SendResponse invokes this on the IO thread. Notifies the backend to
  // handle the actual work of sending the data.
  virtual void SendResponseOnIOThread(
      int request_id,
      scoped_refptr<base::RefCountedMemory> bytes);

  // The name of this source.
  // E.g., for favicons, this could be "favicon", which results in paths for
  // specific resources like "favicon/34" getting sent to this source.
  const std::string source_name_;

  // This field is set and maintained by URLDataManagerBackend. It is set when
  // the DataSource is added, and unset if the DataSource is removed. A
  // DataSource can be removed in two ways: the URLDataManagerBackend is
  // deleted, or another DataSource is registered with the same name. backend_
  // should only be accessed on the IO thread. This reference can't be via a
  // scoped_refptr else there would be a cycle between the backend and data
  // source.
  URLDataManagerBackend* backend_;

  scoped_ptr<URLDataSource> source_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBUI_URL_DATA_SOURCE_IMPL_H_
