/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_I420_MAIN_INTERFACE_I420_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_I420_MAIN_INTERFACE_I420_H_

#include <vector>

#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"
#include "webrtc/typedefs.h"

namespace webrtc {

enum { kI420HeaderSize = 4 };

class I420Encoder : public VideoEncoder {
 public:
  I420Encoder();

  virtual ~I420Encoder();

// Initialize the encoder with the information from the VideoCodec.
//
// Input:
//          - codecSettings     : Codec settings.
//          - numberOfCores     : Number of cores available for the encoder.
//          - maxPayloadSize    : The maximum size each payload is allowed
//                                to have. Usually MTU - overhead.
//
// Return value                 : WEBRTC_VIDEO_CODEC_OK if OK.
//                                <0 - Error
  virtual int InitEncode(const VideoCodec* codecSettings,
                         int /*numberOfCores*/,
                         uint32_t /*maxPayloadSize*/) OVERRIDE;

// "Encode" an I420 image (as a part of a video stream). The encoded image
// will be returned to the user via the encode complete callback.
//
// Input:
//          - inputImage        : Image to be encoded.
//          - codecSpecificInfo : Pointer to codec specific data.
//          - frameType         : Frame type to be sent (Key /Delta).
//
// Return value                 : WEBRTC_VIDEO_CODEC_OK if OK.
//                                <0 - Error
  virtual int Encode(
      const I420VideoFrame& inputImage,
      const CodecSpecificInfo* /*codecSpecificInfo*/,
      const std::vector<VideoFrameType>* /*frame_types*/) OVERRIDE;

// Register an encode complete callback object.
//
// Input:
//          - callback         : Callback object which handles encoded images.
//
// Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
  virtual int RegisterEncodeCompleteCallback(
      EncodedImageCallback* callback) OVERRIDE;

// Free encoder memory.
//
// Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
  virtual int Release() OVERRIDE;

  virtual int SetRates(uint32_t /*newBitRate*/,
                       uint32_t /*frameRate*/) OVERRIDE {
    return WEBRTC_VIDEO_CODEC_OK;
  }

  virtual int SetChannelParameters(uint32_t /*packetLoss*/,
                                   int /*rtt*/) OVERRIDE {
    return WEBRTC_VIDEO_CODEC_OK;
  }

  virtual int CodecConfigParameters(uint8_t* /*buffer*/,
                                    int /*size*/) OVERRIDE {
    return WEBRTC_VIDEO_CODEC_OK;
  }

 private:
  static uint8_t* InsertHeader(uint8_t* buffer, uint16_t width,
                               uint16_t height);

  bool                     _inited;
  EncodedImage             _encodedImage;
  EncodedImageCallback*    _encodedCompleteCallback;
};  // class I420Encoder

class I420Decoder : public VideoDecoder {
 public:
  I420Decoder();

  virtual ~I420Decoder();

// Initialize the decoder.
// The user must notify the codec of width and height values.
//
// Return value         :  WEBRTC_VIDEO_CODEC_OK.
//                        <0 - Errors
  virtual int InitDecode(const VideoCodec* codecSettings,
                         int /*numberOfCores*/) OVERRIDE;

  virtual int SetCodecConfigParameters(const uint8_t* /*buffer*/,
                                       int /*size*/) OVERRIDE {
    return WEBRTC_VIDEO_CODEC_OK;
  }

// Decode encoded image (as a part of a video stream). The decoded image
// will be returned to the user through the decode complete callback.
//
// Input:
//          - inputImage        : Encoded image to be decoded
//          - missingFrames     : True if one or more frames have been lost
//                                since the previous decode call.
//          - codecSpecificInfo : pointer to specific codec data
//          - renderTimeMs      : Render time in Ms
//
// Return value                 : WEBRTC_VIDEO_CODEC_OK if OK
//                                 <0 - Error
  virtual int Decode(const EncodedImage& inputImage,
                     bool missingFrames,
                     const RTPFragmentationHeader* /*fragmentation*/,
                     const CodecSpecificInfo* /*codecSpecificInfo*/,
                     int64_t /*renderTimeMs*/) OVERRIDE;

// Register a decode complete callback object.
//
// Input:
//          - callback         : Callback object which handles decoded images.
//
// Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
  virtual int RegisterDecodeCompleteCallback(
      DecodedImageCallback* callback) OVERRIDE;

// Free decoder memory.
//
// Return value                : WEBRTC_VIDEO_CODEC_OK if OK.
//                                  <0 - Error
  virtual int Release() OVERRIDE;

// Reset decoder state and prepare for a new call.
//
// Return value         :  WEBRTC_VIDEO_CODEC_OK.
//                          <0 - Error
  virtual int Reset() OVERRIDE;

 private:
  static const uint8_t* ExtractHeader(const uint8_t* buffer,
                                      uint16_t* width,
                                      uint16_t* height);

  I420VideoFrame              _decodedImage;
  int                         _width;
  int                         _height;
  bool                        _inited;
  DecodedImageCallback*       _decodeCompleteCallback;
};  // class I420Decoder

}  // namespace webrtc

#endif  // WEBRTC_MODULES_VIDEO_CODING_CODECS_I420_MAIN_INTERFACE_I420_H_
