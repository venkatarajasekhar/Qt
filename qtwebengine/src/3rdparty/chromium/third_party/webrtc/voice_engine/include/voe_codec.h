/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This sub-API supports the following functionalities:
//
//  - Support of non-default codecs (e.g. iLBC, iSAC, etc.).
//  - Voice Activity Detection (VAD) on a per channel basis.
//  - Possibility to specify how to map received payload types to codecs.
//
// Usage example, omitting error checking:
//
//  using namespace webrtc;
//  VoiceEngine* voe = VoiceEngine::Create();
//  VoEBase* base = VoEBase::GetInterface(voe);
//  VoECodec* codec = VoECodec::GetInterface(voe);
//  base->Init();
//  int num_of_codecs = codec->NumOfCodecs()
//  ...
//  base->Terminate();
//  base->Release();
//  codec->Release();
//  VoiceEngine::Delete(voe);
//
#ifndef WEBRTC_VOICE_ENGINE_VOE_CODEC_H
#define WEBRTC_VOICE_ENGINE_VOE_CODEC_H

#include "webrtc/common_types.h"

namespace webrtc {

class VoiceEngine;

class WEBRTC_DLLEXPORT VoECodec
{
public:
    // Factory for the VoECodec sub-API. Increases an internal
    // reference counter if successful. Returns NULL if the API is not
    // supported or if construction fails.
    static VoECodec* GetInterface(VoiceEngine* voiceEngine);

    // Releases the VoECodec sub-API and decreases an internal
    // reference counter. Returns the new reference count. This value should
    // be zero for all sub-API:s before the VoiceEngine object can be safely
    // deleted.
    virtual int Release() = 0;

    // Gets the number of supported codecs.
    virtual int NumOfCodecs() = 0;

    // Get the |codec| information for a specified list |index|.
    virtual int GetCodec(int index, CodecInst& codec) = 0;

    // Sets the |codec| for the |channel| to be used for sending.
    virtual int SetSendCodec(int channel, const CodecInst& codec) = 0;

    // Gets the |codec| parameters for the sending codec on a specified
    // |channel|.
    virtual int GetSendCodec(int channel, CodecInst& codec) = 0;

    // Sets the |codec| as secondary codec for |channel|. Registering a
    // secondary send codec enables dual-streaming. In dual-streaming mode,
    // payloads of the primary and the secondary codecs are packed in RED
    // payloads with |red_payload_type| as payload type. The Secondary codec
    // MUST have the same sampling rate as the primary codec, otherwise the
    // codec cannot be registered and -1 is returned. This method fails if a
    // primary codec is not yet set.
    virtual int SetSecondarySendCodec(int channel, const CodecInst& codec,
                                      int red_payload_type) = 0;

    // Removes the secondary codec from |channel|. This will terminate
    // dual-streaming.
    virtual int RemoveSecondarySendCodec(int channel) = 0;

    // Gets |codec| which is used as secondary codec in |channel|.
    virtual int GetSecondarySendCodec(int channel, CodecInst& codec) = 0;

    // Gets the currently received |codec| for a specific |channel|.
    virtual int GetRecCodec(int channel, CodecInst& codec) = 0;

    // Sets the dynamic payload type number for a particular |codec| or
    // disables (ignores) a codec for receiving. For instance, when receiving
    // an invite from a SIP-based client, this function can be used to change
    // the dynamic payload type number to match that in the INVITE SDP-
    // message. The utilized parameters in the |codec| structure are:
    // plname, plfreq, pltype and channels.
    virtual int SetRecPayloadType(int channel, const CodecInst& codec) = 0;

    // Gets the actual payload type that is set for receiving a |codec| on a
    // |channel|. The value it retrieves will either be the default payload
    // type, or a value earlier set with SetRecPayloadType().
    virtual int GetRecPayloadType(int channel, CodecInst& codec) = 0;

    // Sets the payload |type| for the sending of SID-frames with background
    // noise estimation during silence periods detected by the VAD.
    virtual int SetSendCNPayloadType(
        int channel, int type, PayloadFrequencies frequency = kFreq16000Hz) = 0;

    // Sets the codec internal FEC (forward error correction) status for a
    // specified |channel|. Returns 0 if success, and -1 if failed.
    // TODO(minyue): Make SetFECStatus() pure virtual when fakewebrtcvoiceengine
    // in talk is ready.
    virtual int SetFECStatus(int channel, bool enable) { return -1; }

    // Gets the codec internal FEC status for a specified |channel|. Returns 0
    // with the status stored in |enabled| if success, and -1 if encountered
    // error.
    // TODO(minyue): Make GetFECStatus() pure virtual when fakewebrtcvoiceengine
    // in talk is ready.
    virtual int GetFECStatus(int channel, bool& enabled) { return -1; }

    // Sets the VAD/DTX (silence suppression) status and |mode| for a
    // specified |channel|. Disabling VAD (through |enable|) will also disable
    // DTX; it is not necessary to explictly set |disableDTX| in this case.
    virtual int SetVADStatus(int channel, bool enable,
                             VadModes mode = kVadConventional,
                             bool disableDTX = false) = 0;

    // Gets the VAD/DTX status and |mode| for a specified |channel|.
    virtual int GetVADStatus(int channel, bool& enabled, VadModes& mode,
                             bool& disabledDTX) = 0;

    // Don't use. To be removed.
    virtual int SetAMREncFormat(int channel, AmrMode mode) { return -1; }
    virtual int SetAMRDecFormat(int channel, AmrMode mode) { return -1; }
    virtual int SetAMRWbEncFormat(int channel, AmrMode mode) { return -1; }
    virtual int SetAMRWbDecFormat(int channel, AmrMode mode) { return -1; }
    virtual int SetISACInitTargetRate(int channel, int rateBps,
            bool useFixedFrameSize = false) { return -1; }
    virtual int SetISACMaxRate(int channel, int rateBps) { return -1; }
    virtual int SetISACMaxPayloadSize(int channel, int sizeBytes) { return -1; }

protected:
    VoECodec() {}
    virtual ~VoECodec() {}
};

}  // namespace webrtc

#endif  //  WEBRTC_VOICE_ENGINE_VOE_CODEC_H
