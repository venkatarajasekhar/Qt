// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/decrypting_demuxer_stream.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/decryptor.h"
#include "media/base/demuxer_stream.h"
#include "media/base/pipeline.h"
#include "media/base/video_decoder_config.h"

namespace media {

static bool IsStreamValidAndEncrypted(DemuxerStream* stream) {
  return ((stream->type() == DemuxerStream::AUDIO &&
           stream->audio_decoder_config().IsValidConfig() &&
           stream->audio_decoder_config().is_encrypted()) ||
          (stream->type() == DemuxerStream::VIDEO &&
           stream->video_decoder_config().IsValidConfig() &&
           stream->video_decoder_config().is_encrypted()));
}

DecryptingDemuxerStream::DecryptingDemuxerStream(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const SetDecryptorReadyCB& set_decryptor_ready_cb)
    : task_runner_(task_runner),
      state_(kUninitialized),
      demuxer_stream_(NULL),
      set_decryptor_ready_cb_(set_decryptor_ready_cb),
      decryptor_(NULL),
      key_added_while_decrypt_pending_(false),
      weak_factory_(this) {}

void DecryptingDemuxerStream::Initialize(DemuxerStream* stream,
                                         const PipelineStatusCB& status_cb) {
  DVLOG(2) << __FUNCTION__;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(state_, kUninitialized) << state_;

  DCHECK(!demuxer_stream_);
  weak_this_ = weak_factory_.GetWeakPtr();
  demuxer_stream_ = stream;
  init_cb_ = BindToCurrentLoop(status_cb);

  InitializeDecoderConfig();

  state_ = kDecryptorRequested;
  set_decryptor_ready_cb_.Run(BindToCurrentLoop(
      base::Bind(&DecryptingDemuxerStream::SetDecryptor, weak_this_)));
}

void DecryptingDemuxerStream::Read(const ReadCB& read_cb) {
  DVLOG(3) << __FUNCTION__;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(state_, kIdle) << state_;
  DCHECK(!read_cb.is_null());
  CHECK(read_cb_.is_null()) << "Overlapping reads are not supported.";

  read_cb_ = BindToCurrentLoop(read_cb);
  state_ = kPendingDemuxerRead;
  demuxer_stream_->Read(
      base::Bind(&DecryptingDemuxerStream::DecryptBuffer, weak_this_));
}

void DecryptingDemuxerStream::Reset(const base::Closure& closure) {
  DVLOG(2) << __FUNCTION__ << " - state: " << state_;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(state_ != kUninitialized) << state_;
  DCHECK(state_ != kStopped) << state_;
  DCHECK(reset_cb_.is_null());

  reset_cb_ = BindToCurrentLoop(closure);

  // TODO(xhwang): This should not happen. Remove it, DCHECK against the
  // condition and clean up related tests.
  if (state_ == kDecryptorRequested) {
    DCHECK(!init_cb_.is_null());
    set_decryptor_ready_cb_.Run(DecryptorReadyCB());
    base::ResetAndReturn(&init_cb_).Run(PIPELINE_ERROR_ABORT);
    DoReset();
    return;
  }

  decryptor_->CancelDecrypt(GetDecryptorStreamType());

  // Reset() cannot complete if the read callback is still pending.
  // Defer the resetting process in this case. The |reset_cb_| will be fired
  // after the read callback is fired - see DoDecryptBuffer() and
  // DoDeliverBuffer().
  if (state_ == kPendingDemuxerRead || state_ == kPendingDecrypt) {
    DCHECK(!read_cb_.is_null());
    return;
  }

  if (state_ == kWaitingForKey) {
    DCHECK(!read_cb_.is_null());
    pending_buffer_to_decrypt_ = NULL;
    base::ResetAndReturn(&read_cb_).Run(kAborted, NULL);
  }

  DCHECK(read_cb_.is_null());
  DoReset();
}

void DecryptingDemuxerStream::Stop(const base::Closure& closure) {
  DVLOG(2) << __FUNCTION__ << " - state: " << state_;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(state_ != kUninitialized) << state_;

  // Invalidate all weak pointers so that pending callbacks won't be fired into
  // this object.
  weak_factory_.InvalidateWeakPtrs();

  // At this point the render thread is likely paused (in WebMediaPlayerImpl's
  // Destroy()), so running |closure| can't wait for anything that requires the
  // render thread to process messages to complete (such as PPAPI methods).
  if (decryptor_) {
    decryptor_->CancelDecrypt(GetDecryptorStreamType());
    decryptor_ = NULL;
  }
  if (!set_decryptor_ready_cb_.is_null())
    base::ResetAndReturn(&set_decryptor_ready_cb_).Run(DecryptorReadyCB());
  if (!init_cb_.is_null())
    base::ResetAndReturn(&init_cb_).Run(PIPELINE_ERROR_ABORT);
  if (!read_cb_.is_null())
    base::ResetAndReturn(&read_cb_).Run(kAborted, NULL);
  if (!reset_cb_.is_null())
    base::ResetAndReturn(&reset_cb_).Run();
  pending_buffer_to_decrypt_ = NULL;

  state_ = kStopped;
  BindToCurrentLoop(closure).Run();
}

AudioDecoderConfig DecryptingDemuxerStream::audio_decoder_config() {
  DCHECK(state_ != kUninitialized && state_ != kDecryptorRequested) << state_;
  CHECK_EQ(demuxer_stream_->type(), AUDIO);
  return audio_config_;
}

VideoDecoderConfig DecryptingDemuxerStream::video_decoder_config() {
  DCHECK(state_ != kUninitialized && state_ != kDecryptorRequested) << state_;
  CHECK_EQ(demuxer_stream_->type(), VIDEO);
  return video_config_;
}

DemuxerStream::Type DecryptingDemuxerStream::type() {
  DCHECK(state_ != kUninitialized && state_ != kDecryptorRequested) << state_;
  return demuxer_stream_->type();
}

void DecryptingDemuxerStream::EnableBitstreamConverter() {
  demuxer_stream_->EnableBitstreamConverter();
}

bool DecryptingDemuxerStream::SupportsConfigChanges() {
  return demuxer_stream_->SupportsConfigChanges();
}

DecryptingDemuxerStream::~DecryptingDemuxerStream() {
  DVLOG(2) << __FUNCTION__ << " : state_ = " << state_;
}

void DecryptingDemuxerStream::SetDecryptor(Decryptor* decryptor) {
  DVLOG(2) << __FUNCTION__;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(state_, kDecryptorRequested) << state_;
  DCHECK(!init_cb_.is_null());
  DCHECK(!set_decryptor_ready_cb_.is_null());

  set_decryptor_ready_cb_.Reset();

  if (!decryptor) {
    state_ = kUninitialized;
    base::ResetAndReturn(&init_cb_).Run(DECODER_ERROR_NOT_SUPPORTED);
    return;
  }

  decryptor_ = decryptor;

  decryptor_->RegisterNewKeyCB(
      GetDecryptorStreamType(),
      BindToCurrentLoop(
          base::Bind(&DecryptingDemuxerStream::OnKeyAdded, weak_this_)));

  state_ = kIdle;
  base::ResetAndReturn(&init_cb_).Run(PIPELINE_OK);
}

void DecryptingDemuxerStream::DecryptBuffer(
    DemuxerStream::Status status,
    const scoped_refptr<DecoderBuffer>& buffer) {
  DVLOG(3) << __FUNCTION__;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(state_, kPendingDemuxerRead) << state_;
  DCHECK(!read_cb_.is_null());
  DCHECK_EQ(buffer.get() != NULL, status == kOk) << status;

  // Even when |!reset_cb_.is_null()|, we need to pass |kConfigChanged| back to
  // the caller so that the downstream decoder can be properly reinitialized.
  if (status == kConfigChanged) {
    DVLOG(2) << "DoDecryptBuffer() - kConfigChanged.";
    DCHECK_EQ(demuxer_stream_->type() == AUDIO, audio_config_.IsValidConfig());
    DCHECK_EQ(demuxer_stream_->type() == VIDEO, video_config_.IsValidConfig());

    // Update the decoder config, which the decoder will use when it is notified
    // of kConfigChanged.
    InitializeDecoderConfig();
    state_ = kIdle;
    base::ResetAndReturn(&read_cb_).Run(kConfigChanged, NULL);
    if (!reset_cb_.is_null())
      DoReset();
    return;
  }

  if (!reset_cb_.is_null()) {
    base::ResetAndReturn(&read_cb_).Run(kAborted, NULL);
    DoReset();
    return;
  }

  if (status == kAborted) {
    DVLOG(2) << "DoDecryptBuffer() - kAborted.";
    state_ = kIdle;
    base::ResetAndReturn(&read_cb_).Run(kAborted, NULL);
    return;
  }

  if (buffer->end_of_stream()) {
    DVLOG(2) << "DoDecryptBuffer() - EOS buffer.";
    state_ = kIdle;
    base::ResetAndReturn(&read_cb_).Run(status, buffer);
    return;
  }

  DCHECK(buffer->decrypt_config());
  // An empty iv string signals that the frame is unencrypted.
  if (buffer->decrypt_config()->iv().empty()) {
    DVLOG(2) << "DoDecryptBuffer() - clear buffer.";
    scoped_refptr<DecoderBuffer> decrypted = DecoderBuffer::CopyFrom(
        buffer->data(), buffer->data_size());
    decrypted->set_timestamp(buffer->timestamp());
    decrypted->set_duration(buffer->duration());
    state_ = kIdle;
    base::ResetAndReturn(&read_cb_).Run(kOk, decrypted);
    return;
  }

  pending_buffer_to_decrypt_ = buffer;
  state_ = kPendingDecrypt;
  DecryptPendingBuffer();
}

void DecryptingDemuxerStream::DecryptPendingBuffer() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(state_, kPendingDecrypt) << state_;
  decryptor_->Decrypt(
      GetDecryptorStreamType(),
      pending_buffer_to_decrypt_,
      BindToCurrentLoop(
          base::Bind(&DecryptingDemuxerStream::DeliverBuffer, weak_this_)));
}

void DecryptingDemuxerStream::DeliverBuffer(
    Decryptor::Status status,
    const scoped_refptr<DecoderBuffer>& decrypted_buffer) {
  DVLOG(3) << __FUNCTION__ << " - status: " << status;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(state_, kPendingDecrypt) << state_;
  DCHECK_NE(status, Decryptor::kNeedMoreData);
  DCHECK(!read_cb_.is_null());
  DCHECK(pending_buffer_to_decrypt_.get());

  bool need_to_try_again_if_nokey = key_added_while_decrypt_pending_;
  key_added_while_decrypt_pending_ = false;

  if (!reset_cb_.is_null()) {
    pending_buffer_to_decrypt_ = NULL;
    base::ResetAndReturn(&read_cb_).Run(kAborted, NULL);
    DoReset();
    return;
  }

  DCHECK_EQ(status == Decryptor::kSuccess, decrypted_buffer.get() != NULL);

  if (status == Decryptor::kError) {
    DVLOG(2) << "DoDeliverBuffer() - kError";
    pending_buffer_to_decrypt_ = NULL;
    state_ = kIdle;
    base::ResetAndReturn(&read_cb_).Run(kAborted, NULL);
    return;
  }

  if (status == Decryptor::kNoKey) {
    DVLOG(2) << "DoDeliverBuffer() - kNoKey";
    if (need_to_try_again_if_nokey) {
      // The |state_| is still kPendingDecrypt.
      DecryptPendingBuffer();
      return;
    }

    state_ = kWaitingForKey;
    return;
  }

  DCHECK_EQ(status, Decryptor::kSuccess);
  pending_buffer_to_decrypt_ = NULL;
  state_ = kIdle;
  base::ResetAndReturn(&read_cb_).Run(kOk, decrypted_buffer);
}

void DecryptingDemuxerStream::OnKeyAdded() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (state_ == kPendingDecrypt) {
    key_added_while_decrypt_pending_ = true;
    return;
  }

  if (state_ == kWaitingForKey) {
    state_ = kPendingDecrypt;
    DecryptPendingBuffer();
  }
}

void DecryptingDemuxerStream::DoReset() {
  DCHECK(state_ != kUninitialized);
  DCHECK(init_cb_.is_null());
  DCHECK(read_cb_.is_null());

  if (state_ == kDecryptorRequested)
    state_ = kUninitialized;
  else
    state_ = kIdle;

  base::ResetAndReturn(&reset_cb_).Run();
}

Decryptor::StreamType DecryptingDemuxerStream::GetDecryptorStreamType() const {
  if (demuxer_stream_->type() == AUDIO)
    return Decryptor::kAudio;

  DCHECK_EQ(demuxer_stream_->type(), VIDEO);
  return Decryptor::kVideo;
}

void DecryptingDemuxerStream::InitializeDecoderConfig() {
  // The decoder selector or upstream demuxer make sure the stream is valid and
  // potentially encrypted.
  DCHECK(IsStreamValidAndEncrypted(demuxer_stream_));

  switch (demuxer_stream_->type()) {
    case AUDIO: {
      AudioDecoderConfig input_audio_config =
          demuxer_stream_->audio_decoder_config();
      audio_config_.Initialize(input_audio_config.codec(),
                               input_audio_config.sample_format(),
                               input_audio_config.channel_layout(),
                               input_audio_config.samples_per_second(),
                               input_audio_config.extra_data(),
                               input_audio_config.extra_data_size(),
                               false,  // Output audio is not encrypted.
                               false,
                               base::TimeDelta(),
                               0);
      break;
    }

    case VIDEO: {
      VideoDecoderConfig input_video_config =
          demuxer_stream_->video_decoder_config();
      video_config_.Initialize(input_video_config.codec(),
                               input_video_config.profile(),
                               input_video_config.format(),
                               input_video_config.coded_size(),
                               input_video_config.visible_rect(),
                               input_video_config.natural_size(),
                               input_video_config.extra_data(),
                               input_video_config.extra_data_size(),
                               false,  // Output video is not encrypted.
                               false);
      break;
    }

    default:
      NOTREACHED();
      return;
  }
}

}  // namespace media
