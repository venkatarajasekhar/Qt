; Copyright (c) 2011 The Chromium Authors. All rights reserved.
; Use of this source code is governed by a BSD-style license that can be
; found in the LICENSE file.

%include "third_party/x86inc/x86inc.asm"

;
; This file uses MMX and SSE instructions.
;
  SECTION_TEXT
  CPU       MMX, SSE

; Use movq to save the output.
%define MOVQ movntq

; void ScaleYUVToRGB32Row_SSE(const uint8* y_buf,
;                             const uint8* u_buf,
;                             const uint8* v_buf,
;                             uint8* rgb_buf,
;                             ptrdiff_t width,
;                             ptrdiff_t source_dx);
;                             const int16 convert_table[1024][4]);
%define SYMBOL ScaleYUVToRGB32Row_SSE
%include "scale_yuv_to_rgb_mmx.inc"
