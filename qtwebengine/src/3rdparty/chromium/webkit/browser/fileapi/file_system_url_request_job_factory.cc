// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/browser/fileapi/file_system_url_request_job_factory.h"

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "net/url_request/url_request.h"
#include "webkit/browser/fileapi/file_system_dir_url_request_job.h"
#include "webkit/browser/fileapi/file_system_url_request_job.h"

namespace fileapi {

namespace {

class FileSystemProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  FileSystemProtocolHandler(const std::string& storage_domain,
                            FileSystemContext* context);
  virtual ~FileSystemProtocolHandler();

  virtual net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const OVERRIDE;

 private:
  const std::string storage_domain_;

  // No scoped_refptr because |file_system_context_| is owned by the
  // ProfileIOData, which also owns this ProtocolHandler.
  FileSystemContext* const file_system_context_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemProtocolHandler);
};

FileSystemProtocolHandler::FileSystemProtocolHandler(
    const std::string& storage_domain,
    FileSystemContext* context)
    : storage_domain_(storage_domain),
      file_system_context_(context) {
  DCHECK(file_system_context_);
}

FileSystemProtocolHandler::~FileSystemProtocolHandler() {}

net::URLRequestJob* FileSystemProtocolHandler::MaybeCreateJob(
    net::URLRequest* request, net::NetworkDelegate* network_delegate) const {
  const std::string path = request->url().path();

  // If the path ends with a /, we know it's a directory. If the path refers
  // to a directory and gets dispatched to FileSystemURLRequestJob, that class
  // redirects back here, by adding a / to the URL.
  if (!path.empty() && path[path.size() - 1] == '/') {
    return new FileSystemDirURLRequestJob(
        request, network_delegate, storage_domain_, file_system_context_);
  }
  return new FileSystemURLRequestJob(
      request, network_delegate, storage_domain_, file_system_context_);
}

}  // anonymous namespace

net::URLRequestJobFactory::ProtocolHandler* CreateFileSystemProtocolHandler(
    const std::string& storage_domain, FileSystemContext* context) {
  DCHECK(context);
  return new FileSystemProtocolHandler(storage_domain, context);
}

}  // namespace fileapi
