/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/******************************************************************

 iLBC Speech Coder ANSI-C Source Code

 WebRtcIlbcfix_InitEncode.h

******************************************************************/

#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_INIT_ENCODE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_INIT_ENCODE_H_

#include "defines.h"

/*----------------------------------------------------------------*
 *  Initiation of encoder instance.
 *---------------------------------------------------------------*/

int16_t WebRtcIlbcfix_InitEncode(  /* (o) Number of bytes encoded */
    iLBC_Enc_Inst_t *iLBCenc_inst, /* (i/o) Encoder instance */
    int16_t mode     /* (i) frame size mode */
                                         );

#endif
