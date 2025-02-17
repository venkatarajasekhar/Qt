// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/dom_storage_message_filter.h"

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/sequenced_worker_pool.h"
#include "content/browser/dom_storage/dom_storage_area.h"
#include "content/browser/dom_storage/dom_storage_context_wrapper.h"
#include "content/browser/dom_storage/dom_storage_host.h"
#include "content/browser/dom_storage/dom_storage_namespace.h"
#include "content/browser/dom_storage/dom_storage_task_runner.h"
#include "content/common/dom_storage/dom_storage_messages.h"
#include "content/public/browser/user_metrics.h"
#include "url/gurl.h"

namespace content {

DOMStorageMessageFilter::DOMStorageMessageFilter(
    int render_process_id,
    DOMStorageContextWrapper* context)
    : BrowserMessageFilter(DOMStorageMsgStart),
      render_process_id_(render_process_id),
      context_(context->context()),
      connection_dispatching_message_for_(0) {
}

DOMStorageMessageFilter::~DOMStorageMessageFilter() {
  DCHECK(!host_.get());
}

void DOMStorageMessageFilter::InitializeInSequence() {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  host_.reset(new DOMStorageHost(context_.get(), render_process_id_));
  context_->AddEventObserver(this);
}

void DOMStorageMessageFilter::UninitializeInSequence() {
  // TODO(michaeln): Restore this DCHECK once crbug/166470 and crbug/164403
  // are resolved.
  // DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  context_->RemoveEventObserver(this);
  host_.reset();
}

void DOMStorageMessageFilter::OnFilterAdded(IPC::Sender* sender) {
  context_->task_runner()->PostShutdownBlockingTask(
      FROM_HERE,
      DOMStorageTaskRunner::PRIMARY_SEQUENCE,
      base::Bind(&DOMStorageMessageFilter::InitializeInSequence, this));
}

void DOMStorageMessageFilter::OnFilterRemoved() {
  context_->task_runner()->PostShutdownBlockingTask(
      FROM_HERE,
      DOMStorageTaskRunner::PRIMARY_SEQUENCE,
      base::Bind(&DOMStorageMessageFilter::UninitializeInSequence, this));
}

base::TaskRunner* DOMStorageMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& message) {
  if (IPC_MESSAGE_CLASS(message) == DOMStorageMsgStart)
    return context_->task_runner();
  return NULL;
}

bool DOMStorageMessageFilter::OnMessageReceived(const IPC::Message& message) {
  if (IPC_MESSAGE_CLASS(message) != DOMStorageMsgStart)
    return false;
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(host_.get());

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(DOMStorageMessageFilter, message)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_OpenStorageArea, OnOpenStorageArea)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_CloseStorageArea, OnCloseStorageArea)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_LoadStorageArea, OnLoadStorageArea)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_SetItem, OnSetItem)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_LogGetItem, OnLogGetItem)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_RemoveItem, OnRemoveItem)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_Clear, OnClear)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_FlushMessages, OnFlushMessages)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void DOMStorageMessageFilter::OnOpenStorageArea(int connection_id,
                                                int64 namespace_id,
                                                const GURL& origin) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!host_->OpenStorageArea(connection_id, namespace_id, origin)) {
    RecordAction(base::UserMetricsAction("BadMessageTerminate_DSMF_1"));
    BadMessageReceived();
  }
}

void DOMStorageMessageFilter::OnCloseStorageArea(int connection_id) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  host_->CloseStorageArea(connection_id);
}

void DOMStorageMessageFilter::OnLoadStorageArea(int connection_id,
                                                DOMStorageValuesMap* map,
                                                bool* send_log_get_messages) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!host_->ExtractAreaValues(connection_id, map, send_log_get_messages)) {
    RecordAction(base::UserMetricsAction("BadMessageTerminate_DSMF_2"));
    BadMessageReceived();
  }
  Send(new DOMStorageMsg_AsyncOperationComplete(true));
}

void DOMStorageMessageFilter::OnSetItem(
    int connection_id, const base::string16& key,
    const base::string16& value, const GURL& page_url) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK_EQ(0, connection_dispatching_message_for_);
  base::AutoReset<int> auto_reset(&connection_dispatching_message_for_,
                            connection_id);
  base::NullableString16 not_used;
  bool success = host_->SetAreaItem(connection_id, key, value,
                                    page_url, &not_used);
  Send(new DOMStorageMsg_AsyncOperationComplete(success));
}

void DOMStorageMessageFilter::OnLogGetItem(
    int connection_id, const base::string16& key,
    const base::NullableString16& value) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  host_->LogGetAreaItem(connection_id, key, value);
}

void DOMStorageMessageFilter::OnRemoveItem(
    int connection_id, const base::string16& key,
    const GURL& page_url) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK_EQ(0, connection_dispatching_message_for_);
  base::AutoReset<int> auto_reset(&connection_dispatching_message_for_,
                            connection_id);
  base::string16 not_used;
  host_->RemoveAreaItem(connection_id, key, page_url, &not_used);
  Send(new DOMStorageMsg_AsyncOperationComplete(true));
}

void DOMStorageMessageFilter::OnClear(
    int connection_id, const GURL& page_url) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK_EQ(0, connection_dispatching_message_for_);
  base::AutoReset<int> auto_reset(&connection_dispatching_message_for_,
                            connection_id);
  host_->ClearArea(connection_id, page_url);
  Send(new DOMStorageMsg_AsyncOperationComplete(true));
}

void DOMStorageMessageFilter::OnFlushMessages() {
  // Intentionally empty method body.
}

void DOMStorageMessageFilter::OnDOMStorageItemSet(
    const DOMStorageArea* area,
    const base::string16& key,
    const base::string16& new_value,
    const base::NullableString16& old_value,
    const GURL& page_url) {
  SendDOMStorageEvent(area, page_url,
                      base::NullableString16(key, false),
                      base::NullableString16(new_value, false),
                      old_value);
}

void DOMStorageMessageFilter::OnDOMStorageItemRemoved(
    const DOMStorageArea* area,
    const base::string16& key,
    const base::string16& old_value,
    const GURL& page_url) {
  SendDOMStorageEvent(area, page_url,
                      base::NullableString16(key, false),
                      base::NullableString16(),
                      base::NullableString16(old_value, false));
}

void DOMStorageMessageFilter::OnDOMStorageAreaCleared(
    const DOMStorageArea* area,
    const GURL& page_url) {
  SendDOMStorageEvent(area, page_url,
                      base::NullableString16(),
                      base::NullableString16(),
                      base::NullableString16());
}

void DOMStorageMessageFilter::OnDOMSessionStorageReset(int64 namespace_id) {
  if (host_->ResetOpenAreasForNamespace(namespace_id))
    Send(new DOMStorageMsg_ResetCachedValues(namespace_id));
}

void DOMStorageMessageFilter::SendDOMStorageEvent(
    const DOMStorageArea* area,
    const GURL& page_url,
    const base::NullableString16& key,
    const base::NullableString16& new_value,
    const base::NullableString16& old_value) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  // Only send mutation events to processes which have the area open.
  bool originated_in_process = connection_dispatching_message_for_ != 0;
  int64 alias_namespace_id = area->namespace_id();
  if (host_->HasAreaOpen(area->namespace_id(), area->origin(),
                         &alias_namespace_id) ||
      originated_in_process) {
    DOMStorageMsg_Event_Params params;
    params.origin = area->origin();
    params.page_url = page_url;
    params.connection_id = connection_dispatching_message_for_;
    params.key = key;
    params.new_value = new_value;
    params.old_value = old_value;
    params.namespace_id = alias_namespace_id;
    Send(new DOMStorageMsg_Event(params));
  }
}

}  // namespace content
