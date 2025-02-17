// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_VIDEO_VIDEO_DECODE_ACCELERATOR_H_
#define MEDIA_VIDEO_VIDEO_DECODE_ACCELERATOR_H_

#include <vector>

#include "base/basictypes.h"
#include "media/base/bitstream_buffer.h"
#include "media/base/video_decoder_config.h"
#include "media/video/picture.h"
#include "ui/gfx/size.h"

namespace media {

// Video decoder interface.
// This interface is extended by the various components that ultimately
// implement the backend of PPB_VideoDecode_Dev.
class MEDIA_EXPORT VideoDecodeAccelerator {
 public:
  // Enumeration of potential errors generated by the API.
  // Note: Keep these in sync with PP_VideoDecodeError_Dev. Also do not
  // rearrange, reuse or remove values as they are used for gathering UMA
  // statistics.
  enum Error {
    // An operation was attempted during an incompatible decoder state.
    ILLEGAL_STATE = 1,
    // Invalid argument was passed to an API method.
    INVALID_ARGUMENT,
    // Encoded input is unreadable.
    UNREADABLE_INPUT,
    // A failure occurred at the browser layer or one of its dependencies.
    // Examples of such failures include GPU hardware failures, GPU driver
    // failures, GPU library failures, browser programming errors, and so on.
    PLATFORM_FAILURE,
    // Largest used enum. This should be adjusted when new errors are added.
    LARGEST_ERROR_ENUM,
  };

  // Interface for collaborating with picture interface to provide memory for
  // output picture and blitting them. These callbacks will not be made unless
  // Initialize() has returned successfully.
  // This interface is extended by the various layers that relay messages back
  // to the plugin, through the PPP_VideoDecode_Dev interface the plugin
  // implements.
  class MEDIA_EXPORT Client {
   public:
    // Callback to tell client how many and what size of buffers to provide.
    virtual void ProvidePictureBuffers(uint32 requested_num_of_buffers,
                                       const gfx::Size& dimensions,
                                       uint32 texture_target) = 0;

    // Callback to dismiss picture buffer that was assigned earlier.
    virtual void DismissPictureBuffer(int32 picture_buffer_id) = 0;

    // Callback to deliver decoded pictures ready to be displayed.
    virtual void PictureReady(const Picture& picture) = 0;

    // Callback to notify that decoded has decoded the end of the current
    // bitstream buffer.
    virtual void NotifyEndOfBitstreamBuffer(int32 bitstream_buffer_id) = 0;

    // Flush completion callback.
    virtual void NotifyFlushDone() = 0;

    // Reset completion callback.
    virtual void NotifyResetDone() = 0;

    // Callback to notify about decoding errors. Note that errors in
    // Initialize() will not be reported here, but will instead be indicated by
    // a false return value there.
    virtual void NotifyError(Error error) = 0;

   protected:
    virtual ~Client() {}
  };

  // Video decoder functions.

  // Initializes the video decoder with specific configuration.  Called once per
  // decoder construction.  This call is synchronous and returns true iff
  // initialization is successful.
  // Parameters:
  //  |profile| is the video stream's format profile.
  //  |client| is the client of this video decoder.  The provided pointer must
  //  be valid until Destroy() is called.
  virtual bool Initialize(VideoCodecProfile profile, Client* client) = 0;

  // Decodes given bitstream buffer that contains at most one frame.  Once
  // decoder is done with processing |bitstream_buffer| it will call
  // NotifyEndOfBitstreamBuffer() with the bitstream buffer id.
  // Parameters:
  //  |bitstream_buffer| is the input bitstream that is sent for decoding.
  virtual void Decode(const BitstreamBuffer& bitstream_buffer) = 0;

  // Assigns a set of texture-backed picture buffers to the video decoder.
  //
  // Ownership of each picture buffer remains with the client, but the client
  // is not allowed to deallocate the buffer before the DismissPictureBuffer
  // callback has been initiated for a given buffer.
  //
  // Parameters:
  //  |buffers| contains the allocated picture buffers for the output.
  virtual void AssignPictureBuffers(
      const std::vector<PictureBuffer>& buffers) = 0;

  // Sends picture buffers to be reused by the decoder. This needs to be called
  // for each buffer that has been processed so that decoder may know onto which
  // picture buffers it can write the output to.
  //
  // Parameters:
  //  |picture_buffer_id| id of the picture buffer that is to be reused.
  virtual void ReusePictureBuffer(int32 picture_buffer_id) = 0;

  // Flushes the decoder: all pending inputs will be decoded and pictures handed
  // back to the client, followed by NotifyFlushDone() being called on the
  // client.  Can be used to implement "end of stream" notification.
  virtual void Flush() = 0;

  // Resets the decoder: all pending inputs are dropped immediately and the
  // decoder returned to a state ready for further Decode()s, followed by
  // NotifyResetDone() being called on the client.  Can be used to implement
  // "seek".
  virtual void Reset() = 0;

  // Destroys the decoder: all pending inputs are dropped immediately and the
  // component is freed.  This call may asynchornously free system resources,
  // but its client-visible effects are synchronous.  After this method returns
  // no more callbacks will be made on the client.  Deletes |this|
  // unconditionally, so make sure to drop all pointers to it!
  virtual void Destroy() = 0;

  // GPU PROCESS ONLY.  Implementations of this interface in the
  // content/common/gpu/media should implement this, and implementations in
  // other processes should not override the default implementation.
  // Returns true if VDA::Decode and VDA::Client callbacks can run on the IO
  // thread. Otherwise they will run on the GPU child thread. The purpose of
  // running Decode on the IO thread is to reduce decode latency. Note Decode
  // should return as soon as possible and not block on the IO thread. Also,
  // PictureReady should be run on the child thread if a picture is delivered
  // the first time so it can be cleared.
  virtual bool CanDecodeOnIOThread();

 protected:
  // Do not delete directly; use Destroy() or own it with a scoped_ptr, which
  // will Destroy() it properly by default.
  virtual ~VideoDecodeAccelerator();
};

}  // namespace media

namespace base {

template <class T>
struct DefaultDeleter;

// Specialize DefaultDeleter so that scoped_ptr<VideoDecodeAccelerator> always
// uses "Destroy()" instead of trying to use the destructor.
template <>
struct MEDIA_EXPORT DefaultDeleter<media::VideoDecodeAccelerator> {
 public:
  void operator()(void* video_decode_accelerator) const;
};

}  // namespace base

#endif  // MEDIA_VIDEO_VIDEO_DECODE_ACCELERATOR_H_
