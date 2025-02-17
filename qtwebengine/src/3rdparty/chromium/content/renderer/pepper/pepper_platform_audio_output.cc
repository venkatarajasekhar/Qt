// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_platform_audio_output.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop_proxy.h"
#include "build/build_config.h"
#include "content/child/child_process.h"
#include "content/common/media/audio_messages.h"
#include "content/renderer/media/audio_message_filter.h"
#include "content/renderer/pepper/audio_helper.h"
#include "content/renderer/render_thread_impl.h"
#include "media/base/audio_hardware_config.h"
#include "ppapi/shared_impl/ppb_audio_config_shared.h"

namespace content {

// static
PepperPlatformAudioOutput* PepperPlatformAudioOutput::Create(
    int sample_rate,
    int frames_per_buffer,
    int source_render_view_id,
    int source_render_frame_id,
    AudioHelper* client) {
  scoped_refptr<PepperPlatformAudioOutput> audio_output(
      new PepperPlatformAudioOutput());
  if (audio_output->Initialize(sample_rate,
                               frames_per_buffer,
                               source_render_view_id,
                               source_render_frame_id,
                               client)) {
    // Balanced by Release invoked in
    // PepperPlatformAudioOutput::ShutDownOnIOThread().
    audio_output->AddRef();
    return audio_output.get();
  }
  return NULL;
}

bool PepperPlatformAudioOutput::StartPlayback() {
  if (ipc_) {
    io_message_loop_proxy_->PostTask(
        FROM_HERE,
        base::Bind(&PepperPlatformAudioOutput::StartPlaybackOnIOThread, this));
    return true;
  }
  return false;
}

bool PepperPlatformAudioOutput::StopPlayback() {
  if (ipc_) {
    io_message_loop_proxy_->PostTask(
        FROM_HERE,
        base::Bind(&PepperPlatformAudioOutput::StopPlaybackOnIOThread, this));
    return true;
  }
  return false;
}

void PepperPlatformAudioOutput::ShutDown() {
  // Called on the main thread to stop all audio callbacks. We must only change
  // the client on the main thread, and the delegates from the I/O thread.
  client_ = NULL;
  io_message_loop_proxy_->PostTask(
      FROM_HERE,
      base::Bind(&PepperPlatformAudioOutput::ShutDownOnIOThread, this));
}

void PepperPlatformAudioOutput::OnStateChanged(
    media::AudioOutputIPCDelegate::State state) {}

void PepperPlatformAudioOutput::OnStreamCreated(
    base::SharedMemoryHandle handle,
    base::SyncSocket::Handle socket_handle,
    int length) {
#if defined(OS_WIN)
  DCHECK(handle);
  DCHECK(socket_handle);
#else
  DCHECK_NE(-1, handle.fd);
  DCHECK_NE(-1, socket_handle);
#endif
  DCHECK(length);

  if (base::MessageLoopProxy::current().get() ==
      main_message_loop_proxy_.get()) {
    // Must dereference the client only on the main thread. Shutdown may have
    // occurred while the request was in-flight, so we need to NULL check.
    if (client_)
      client_->StreamCreated(handle, length, socket_handle);
  } else {
    main_message_loop_proxy_->PostTask(
        FROM_HERE,
        base::Bind(&PepperPlatformAudioOutput::OnStreamCreated,
                   this,
                   handle,
                   socket_handle,
                   length));
  }
}

void PepperPlatformAudioOutput::OnIPCClosed() { ipc_.reset(); }

PepperPlatformAudioOutput::~PepperPlatformAudioOutput() {
  // Make sure we have been shut down. Warning: this will usually happen on
  // the I/O thread!
  DCHECK(!ipc_);
  DCHECK(!client_);
}

PepperPlatformAudioOutput::PepperPlatformAudioOutput()
    : client_(NULL),
      main_message_loop_proxy_(base::MessageLoopProxy::current()),
      io_message_loop_proxy_(ChildProcess::current()->io_message_loop_proxy()) {
}

bool PepperPlatformAudioOutput::Initialize(int sample_rate,
                                           int frames_per_buffer,
                                           int source_render_view_id,
                                           int source_render_frame_id,
                                           AudioHelper* client) {
  DCHECK(client);
  client_ = client;

  RenderThreadImpl* const render_thread = RenderThreadImpl::current();
  ipc_ = render_thread->audio_message_filter()->CreateAudioOutputIPC(
      source_render_view_id, source_render_frame_id);
  CHECK(ipc_);

  media::AudioParameters params(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                                media::CHANNEL_LAYOUT_STEREO,
                                sample_rate,
                                ppapi::kBitsPerAudioOutputSample,
                                frames_per_buffer);

  io_message_loop_proxy_->PostTask(
      FROM_HERE,
      base::Bind(
          &PepperPlatformAudioOutput::InitializeOnIOThread, this, params));
  return true;
}

void PepperPlatformAudioOutput::InitializeOnIOThread(
    const media::AudioParameters& params) {
  DCHECK(io_message_loop_proxy_->BelongsToCurrentThread());
  const int kSessionId = 0;
  if (ipc_)
    ipc_->CreateStream(this, params, kSessionId);
}

void PepperPlatformAudioOutput::StartPlaybackOnIOThread() {
  DCHECK(io_message_loop_proxy_->BelongsToCurrentThread());
  if (ipc_)
    ipc_->PlayStream();
}

void PepperPlatformAudioOutput::StopPlaybackOnIOThread() {
  DCHECK(io_message_loop_proxy_->BelongsToCurrentThread());
  if (ipc_)
    ipc_->PauseStream();
}

void PepperPlatformAudioOutput::ShutDownOnIOThread() {
  DCHECK(io_message_loop_proxy_->BelongsToCurrentThread());

  // Make sure we don't call shutdown more than once.
  if (!ipc_)
    return;

  ipc_->CloseStream();
  ipc_.reset();

  Release();  // Release for the delegate, balances out the reference taken in
              // PepperPlatformAudioOutput::Create.
}

}  // namespace content
