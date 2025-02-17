// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webui/url_data_manager.h"

#include <vector>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/memory/ref_counted_memory.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_util.h"
#include "base/synchronization/lock.h"
#include "content/browser/resource_context_impl.h"
#include "content/browser/webui/url_data_manager_backend.h"
#include "content/browser/webui/url_data_source_impl.h"
#include "content/browser/webui/web_ui_data_source_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/url_data_source.h"

namespace content {
namespace {

const char kURLDataManagerKeyName[] = "url_data_manager";

base::LazyInstance<base::Lock>::Leaky g_delete_lock = LAZY_INSTANCE_INITIALIZER;

URLDataManager* GetFromBrowserContext(BrowserContext* context) {
  if (!context->GetUserData(kURLDataManagerKeyName)) {
    context->SetUserData(kURLDataManagerKeyName, new URLDataManager(context));
  }
  return static_cast<URLDataManager*>(
      context->GetUserData(kURLDataManagerKeyName));
}

// Invoked on the IO thread to do the actual adding of the DataSource.
static void AddDataSourceOnIOThread(
    ResourceContext* resource_context,
    scoped_refptr<URLDataSourceImpl> data_source) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  GetURLDataManagerForResourceContext(resource_context)->AddDataSource(
      data_source.get());
}

}  // namespace

// static
URLDataManager::URLDataSources* URLDataManager::data_sources_ = NULL;

URLDataManager::URLDataManager(BrowserContext* browser_context)
    : browser_context_(browser_context) {
}

URLDataManager::~URLDataManager() {
}

void URLDataManager::AddDataSource(URLDataSourceImpl* source) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&AddDataSourceOnIOThread,
                 browser_context_->GetResourceContext(),
                 make_scoped_refptr(source)));
}

// static
void URLDataManager::DeleteDataSources() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  URLDataSources sources;
  {
    base::AutoLock lock(g_delete_lock.Get());
    if (!data_sources_)
      return;
    data_sources_->swap(sources);
  }
  for (size_t i = 0; i < sources.size(); ++i)
    delete sources[i];
}

// static
void URLDataManager::DeleteDataSource(const URLDataSourceImpl* data_source) {
  // Invoked when a DataSource is no longer referenced and needs to be deleted.
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    // We're on the UI thread, delete right away.
    delete data_source;
    return;
  }

  // We're not on the UI thread, add the DataSource to the list of DataSources
  // to delete.
  bool schedule_delete = false;
  {
    base::AutoLock lock(g_delete_lock.Get());
    if (!data_sources_)
      data_sources_ = new URLDataSources();
    schedule_delete = data_sources_->empty();
    data_sources_->push_back(data_source);
  }
  if (schedule_delete) {
    // Schedule a task to delete the DataSource back on the UI thread.
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&URLDataManager::DeleteDataSources));
  }
}

// static
void URLDataManager::AddDataSource(BrowserContext* browser_context,
                                   URLDataSource* source) {
  GetFromBrowserContext(browser_context)->
      AddDataSource(new URLDataSourceImpl(source->GetSource(), source));
}

// static
void URLDataManager::AddWebUIDataSource(BrowserContext* browser_context,
                                        WebUIDataSource* source) {
  WebUIDataSourceImpl* impl = static_cast<WebUIDataSourceImpl*>(source);
  GetFromBrowserContext(browser_context)->AddDataSource(impl);
}

// static
bool URLDataManager::IsScheduledForDeletion(
    const URLDataSourceImpl* data_source) {
  base::AutoLock lock(g_delete_lock.Get());
  if (!data_sources_)
    return false;
  return std::find(data_sources_->begin(), data_sources_->end(), data_source) !=
      data_sources_->end();
}

}  // namespace content
