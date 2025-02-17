// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_media_stream_track_host_base.h"

#include "base/logging.h"
#include "base/numerics/safe_math.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/media_stream_buffer.h"

using ppapi::host::HostMessageContext;
using ppapi::proxy::SerializedHandle;

namespace content {

PepperMediaStreamTrackHostBase::PepperMediaStreamTrackHostBase(
    RendererPpapiHost* host,
    PP_Instance instance,
    PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      host_(host),
      buffer_manager_(this) {}

PepperMediaStreamTrackHostBase::~PepperMediaStreamTrackHostBase() {}

bool PepperMediaStreamTrackHostBase::InitBuffers(int32_t number_of_buffers,
                                                 int32_t buffer_size,
                                                 TrackType track_type) {
  DCHECK_GT(number_of_buffers, 0);
  DCHECK_GT(buffer_size,
            static_cast<int32_t>(sizeof(ppapi::MediaStreamBuffer::Header)));
  // Make each buffer 4 byte aligned.
  base::CheckedNumeric<int32_t> buffer_size_aligned = buffer_size;
  buffer_size_aligned += (4 - buffer_size % 4);

  // TODO(penghuang): |HostAllocateSharedMemoryBuffer| uses sync IPC. We should
  // avoid it.
  base::CheckedNumeric<int32_t> size = number_of_buffers * buffer_size_aligned;
  if (!size.IsValid())
    return false;

  content::RenderThread* render_thread = content::RenderThread::Get();
  scoped_ptr<base::SharedMemory> shm(
      render_thread->HostAllocateSharedMemoryBuffer(size.ValueOrDie()).Pass());
  if (!shm)
    return false;

  base::SharedMemoryHandle shm_handle = shm->handle();
  if (!buffer_manager_.SetBuffers(number_of_buffers,
                                  buffer_size_aligned.ValueOrDie(),
                                  shm.Pass(),
                                  true)) {
    return false;
  }

  base::PlatformFile platform_file =
#if defined(OS_WIN)
      shm_handle;
#elif defined(OS_POSIX)
      shm_handle.fd;
#else
#error Not implemented.
#endif
  SerializedHandle handle(host_->ShareHandleWithRemote(platform_file, false),
                          size.ValueOrDie());
  bool readonly = (track_type == kRead);
  host()->SendUnsolicitedReplyWithHandles(
      pp_resource(),
      PpapiPluginMsg_MediaStreamTrack_InitBuffers(
          number_of_buffers,
          buffer_size_aligned.ValueOrDie(),
          readonly),
      std::vector<SerializedHandle>(1, handle));
  return true;
}

void PepperMediaStreamTrackHostBase::SendEnqueueBufferMessageToPlugin(
    int32_t index) {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, buffer_manager_.number_of_buffers());
  host()->SendUnsolicitedReply(
      pp_resource(), PpapiPluginMsg_MediaStreamTrack_EnqueueBuffer(index));
}

void PepperMediaStreamTrackHostBase::SendEnqueueBuffersMessageToPlugin(
    const std::vector<int32_t>& indices) {
  DCHECK_GE(indices.size(), 0U);
  host()->SendUnsolicitedReply(pp_resource(),
      PpapiPluginMsg_MediaStreamTrack_EnqueueBuffers(indices));
}

int32_t PepperMediaStreamTrackHostBase::OnResourceMessageReceived(
    const IPC::Message& msg,
    HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperMediaStreamTrackHostBase, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(
        PpapiHostMsg_MediaStreamTrack_EnqueueBuffer, OnHostMsgEnqueueBuffer)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_MediaStreamTrack_Close,
                                        OnHostMsgClose)
  PPAPI_END_MESSAGE_MAP()
  return ppapi::host::ResourceHost::OnResourceMessageReceived(msg, context);
}

int32_t PepperMediaStreamTrackHostBase::OnHostMsgEnqueueBuffer(
    HostMessageContext* context,
    int32_t index) {
  buffer_manager_.EnqueueBuffer(index);
  return PP_OK;
}

int32_t PepperMediaStreamTrackHostBase::OnHostMsgClose(
    HostMessageContext* context) {
  OnClose();
  return PP_OK;
}

}  // namespace content
