; /*
; * Provide SSE luma and chroma mc functions for HEVC decoding
; * Copyright (c) 2013 Pierre-Edouard LEPERE
; *
; * This file is part of FFmpeg.
; *
; * FFmpeg is free software; you can redistribute it and/or
; * modify it under the terms of the GNU Lesser General Public
; * License as published by the Free Software Foundation; either
; * version 2.1 of the License, or (at your option) any later version.
; *
; * FFmpeg is distributed in the hope that it will be useful,
; * but WITHOUT ANY WARRANTY; without even the implied warranty of
; * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; * Lesser General Public License for more details.
; *
; * You should have received a copy of the GNU Lesser General Public
; * License along with FFmpeg; if not, write to the Free Software
; * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
; */
%include "libavutil/x86/x86util.asm"

SECTION_RODATA
pw_8:                   times 8 dw 512
pw_10:                  times 8 dw 2048
pw_bi_8:                times 8 dw 256
pw_bi_10:               times 8 dw 1024
max_pixels_10:          times 8  dw 1023
zero:                   times 4  dd 0
one_per_32:             times 4  dd 1

SECTION .text
%macro EPEL_TABLE 4
hevc_epel_filters_%4_%1 times %2 d%3 -2, 58
                        times %2 d%3 10, -2
                        times %2 d%3 -4, 54
                        times %2 d%3 16, -2
                        times %2 d%3 -6, 46
                        times %2 d%3 28, -4
                        times %2 d%3 -4, 36
                        times %2 d%3 36, -4
                        times %2 d%3 -4, 28
                        times %2 d%3 46, -6
                        times %2 d%3 -2, 16
                        times %2 d%3 54, -4
                        times %2 d%3 -2, 10
                        times %2 d%3 58, -2
%endmacro



EPEL_TABLE  8, 8, b, sse4
EPEL_TABLE 10, 4, w, sse4

%macro QPEL_TABLE 4
hevc_qpel_filters_%4_%1 times %2 d%3  -1,  4
                        times %2 d%3 -10, 58
                        times %2 d%3  17, -5
                        times %2 d%3   1,  0
                        times %2 d%3  -1,  4
                        times %2 d%3 -11, 40
                        times %2 d%3  40,-11
                        times %2 d%3   4, -1
                        times %2 d%3   0,  1
                        times %2 d%3  -5, 17
                        times %2 d%3  58,-10
                        times %2 d%3   4, -1
%endmacro

QPEL_TABLE  8, 8, b, sse4
QPEL_TABLE 10, 4, w, sse4

%define hevc_qpel_filters_sse4_14 hevc_qpel_filters_sse4_10

%if ARCH_X86_64

%macro SIMPLE_BILOAD 4   ;width, tab, r1, r2
%if %1 <= 4
    movq              %3, [%2]                                              ; load data from source2
%elif %1 <= 8
    movdqa            %3, [%2]                                              ; load data from source2
%elif %1 <= 12
    movdqa            %3, [%2]                                              ; load data from source2
    movq              %4, [%2+16]                                           ; load data from source2
%else
    movdqa            %3, [%2]                                              ; load data from source2
    movdqa            %4, [%2+16]                                           ; load data from source2
%endif
%endmacro

%macro SIMPLE_LOAD 4    ;width, bitd, tab, r1
%if %1 == 2 || (%2 == 8 && %1 <= 4)
    movd              %4, [%3]                                               ; load data from source
%elif %1 == 4 || (%2 == 8 && %1 <= 8)
    movq              %4, [%3]                                               ; load data from source
%else
    movdqu            %4, [%3]                                               ; load data from source
%endif
%endmacro

%macro SIMPLE_8LOAD 5    ;width, bitd, tab, r1, r2
%if %1 == 2 || (%2 == 8 && %1 <= 4)
    movq              %4, [%3]                                              ; load data from source2
%elif %1 == 4 || (%2 == 8 && %1 <= 8)
    movdqa            %4, [%3]                                              ; load data from source2
%elif %1 <= 12
    movdqa            %4, [%3]                                              ; load data from source2
    movq              %5, [%3+16]                                           ; load data from source2
%else
    movdqa            %4, [%3]                                              ; load data from source2
    movdqa            %5, [%3+16]                                           ; load data from source2
%endif
%endmacro

%macro EPEL_FILTER 2                             ; bit depth, filter index
%ifdef PIC
    lea         rfilterq, [hevc_epel_filters_sse4_%1]
%else
    %define rfilterq hevc_epel_filters_sse4_%1
%endif
    sub              %2q, 1
    shl              %2q, 5                      ; multiply by 32
    movdqa           m14, [rfilterq + %2q]        ; get 2 first values of filters
    movdqa           m15, [rfilterq + %2q+16]     ; get 2 last values of filters
%endmacro

%macro EPEL_HV_FILTER 1
%ifdef PIC
    lea         rfilterq, [hevc_epel_filters_sse4_%1]
%else
    %define rfilterq hevc_epel_filters_sse4_%1
%endif
    sub              mxq, 1
    sub              myq, 1
    shl              mxq, 5                      ; multiply by 32
    shl              myq, 5                      ; multiply by 32
    movdqa           m14, [rfilterq + mxq]        ; get 2 first values of filters
    movdqa           m15, [rfilterq + mxq+16]     ; get 2 last values of filters
    lea           r3srcq, [srcstrideq*3]

%ifdef PIC
    lea         rfilterq, [hevc_epel_filters_sse4_10]
%else
    %define rfilterq hevc_epel_filters_sse4_10
%endif
    movdqa           m12, [rfilterq + myq]        ; get 2 first values of filters
    movdqa           m13, [rfilterq + myq+16]     ; get 2 last values of filters
%endmacro

%macro QPEL_FILTER 2
%ifdef PIC
    lea         rfilterq, [hevc_qpel_filters_sse4_%1]
%else
    %define rfilterq hevc_qpel_filters_sse4_%1
%endif
    lea              %2q, [%2q*8-8]
    movdqa           m12, [rfilterq + %2q*8]       ; get 4 first values of filters
    movdqa           m13, [rfilterq + %2q*8 + 16]  ; get 4 first values of filters
    movdqa           m14, [rfilterq + %2q*8 + 32]  ; get 4 first values of filters
    movdqa           m15, [rfilterq + %2q*8 + 48]  ; get 4 first values of filters
%endmacro

%macro EPEL_LOAD 4
%ifdef PIC
    lea rfilterq, [%2]
%else
    %define rfilterq %2
%endif
    movdqu            m0, [rfilterq ]            ;load 128bit of x
%ifnum %3
    movdqu            m1, [rfilterq+  %3]        ;load 128bit of x+stride
    movdqu            m2, [rfilterq+2*%3]        ;load 128bit of x+2*stride
    movdqu            m3, [rfilterq+3*%3]        ;load 128bit of x+3*stride
%else
    movdqu            m1, [rfilterq+  %3q]       ;load 128bit of x+stride
    movdqu            m2, [rfilterq+2*%3q]       ;load 128bit of x+2*stride
    movdqu            m3, [rfilterq+r3srcq]      ;load 128bit of x+2*stride
%endif

%if %1 == 8
%if %4 > 8
    SBUTTERFLY        bw, 0, 1, 10
    SBUTTERFLY        bw, 2, 3, 10
%else
    punpcklbw         m0, m1
    punpcklbw         m2, m3
%endif
%else
%if %4 > 4
    SBUTTERFLY        wd, 0, 1, 10
    SBUTTERFLY        wd, 2, 3, 10
%else
    punpcklwd         m0, m1
    punpcklwd         m2, m3
%endif
%endif
%endmacro


%macro QPEL_H_LOAD 4
%assign %%stride (%1+7)/8
%if %1 == 8
%if %3 <= 4
%define %%load movd
%elif %3 == 8
%define %%load movq
%else
%define %%load movdqu
%endif
%else
%if %3 == 2
%define %%load movd
%elif %3 == 4
%define %%load movq
%else
%define %%load movdqu
%endif
%endif
    %%load            m0, [%2-3*%%stride]        ;load data from source
    %%load            m1, [%2-2*%%stride]
    %%load            m2, [%2-%%stride  ]
    %%load            m3, [%2           ]
    %%load            m4, [%2+%%stride  ]
    %%load            m5, [%2+2*%%stride]
    %%load            m6, [%2+3*%%stride]
    %%load            m7, [%2+4*%%stride]

%if %1 == 8
%if %3 > 8
    SBUTTERFLY        wd, 0, 1, %4
    SBUTTERFLY        wd, 2, 3, %4
    SBUTTERFLY        wd, 4, 5, %4
    SBUTTERFLY        wd, 6, 7, %4
%else
    punpcklwd         m0, m1
    punpcklwd         m2, m3
    punpcklwd         m4, m5
    punpcklwd         m6, m7
%endif
%else
%if %3 > 4
    SBUTTERFLY        dq, 0, 1, %4
    SBUTTERFLY        dq, 2, 3, %4
    SBUTTERFLY        dq, 4, 5, %4
    SBUTTERFLY        dq, 6, 7, %4
%else
    punpckldq         m0, m1
    punpckldq         m2, m3
    punpckldq         m4, m5
    punpckldq         m6, m7
%endif
%endif
%endmacro

%macro QPEL_V_LOAD 4
    lea             r12q, [%2]
    sub             r12q, r3srcq
    movdqu            m0, [r12            ]      ;load x- 3*srcstride
    movdqu            m1, [r12+   %3q     ]      ;load x- 2*srcstride
    movdqu            m2, [r12+ 2*%3q     ]      ;load x-srcstride
    movdqu            m3, [%2       ]      ;load x
    movdqu            m4, [%2+   %3q]      ;load x+stride
    movdqu            m5, [%2+ 2*%3q]      ;load x+2*stride
    movdqu            m6, [%2+r3srcq]      ;load x+3*stride
    movdqu            m7, [%2+ 4*%3q]      ;load x+4*stride
%if %1 == 8
%if %4 > 8
    SBUTTERFLY        bw, 0, 1, 8
    SBUTTERFLY        bw, 2, 3, 8
    SBUTTERFLY        bw, 4, 5, 8
    SBUTTERFLY        bw, 6, 7, 8
%else
    punpcklbw         m0, m1
    punpcklbw         m2, m3
    punpcklbw         m4, m5
    punpcklbw         m6, m7
%endif
%else
%if %4 > 4
    SBUTTERFLY        wd, 0, 1, 8
    SBUTTERFLY        wd, 2, 3, 8
    SBUTTERFLY        wd, 4, 5, 8
    SBUTTERFLY        wd, 6, 7, 8
%else
    punpcklwd         m0, m1
    punpcklwd         m2, m3
    punpcklwd         m4, m5
    punpcklwd         m6, m7
%endif
%endif
%endmacro

%macro PEL_10STORE2 3
    movd           [%1], %2
%endmacro
%macro PEL_10STORE4 3
    movq           [%1], %2
%endmacro
%macro PEL_10STORE6 3
    movq           [%1], %2
    psrldq            %2, 8
    movd         [%1+8], %2
%endmacro
%macro PEL_10STORE8 3
    movdqa         [%1], %2
%endmacro
%macro PEL_10STORE12 3
    movdqa         [%1], %2
    movq        [%1+16], %3
%endmacro
%macro PEL_10STORE16 3
    PEL_10STORE8      %1, %2, %3
    movdqa       [%1+16], %3
%endmacro

%macro PEL_8STORE2 3
    pextrw          [%1], %2, 0
%endmacro
%macro PEL_8STORE4 3
    movd            [%1], %2
%endmacro
%macro PEL_8STORE6 3
    movd            [%1], %2
    pextrw        [%1+4], %2, 2
%endmacro
%macro PEL_8STORE8 3
    movq           [%1], %2
%endmacro
%macro PEL_8STORE12 3
    movq            [%1], %2
    psrldq            %2, 8
    movd          [%1+8], %2
%endmacro
%macro PEL_8STORE16 3
    movdqa          [%1], %2
%endmacro

%macro LOOP_END 4
    lea              %1q, [%1q+2*%2q]            ; dst += dststride
    lea              %3q, [%3q+  %4q]            ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
%endmacro


%macro MC_PIXEL_COMPUTE 2 ;width, bitdepth
%if %2 == 8
%if %1 > 8
    punpckhbw         m1, m0, m2
    psllw             m1, 14-%2
%endif
    punpcklbw         m0, m2
%endif
    psllw             m0, 14-%2
%endmacro


%macro EPEL_COMPUTE 4 ; bitdepth, width, filter1, filter2
%if %1 == 8
    pmaddubsw         m0, %3   ;x1*c1+x2*c2
    pmaddubsw         m2, %4   ;x3*c3+x4*c4
    paddw             m0, m2
%if %2 > 8
    pmaddubsw         m1, %3
    pmaddubsw         m3, %4
    paddw             m1, m3
%endif
%else
    pmaddwd           m0, %3
    pmaddwd           m2, %4
    paddd             m0, m2
%if %2 > 4
    pmaddwd           m1, %3
    pmaddwd           m3, %4
    paddd             m1, m3
%endif
    psrad             m0, %1-8
    psrad             m1, %1-8
    packssdw          m0, m1
%endif
%endmacro

%macro QPEL_HV_COMPUTE 4     ; width, bitdepth, filter idx
%ifdef PIC
    lea         rfilterq, [hevc_qpel_filters_sse4_%2]
%else
    %define rfilterq hevc_qpel_filters_sse4_%2
%endif

%if %2 == 8
    pmaddubsw         m0, [rfilterq + %3q*8   ]   ;x1*c1+x2*c2
    pmaddubsw         m2, [rfilterq + %3q*8+16]   ;x3*c3+x4*c4
    pmaddubsw         m4, [rfilterq + %3q*8+32]   ;x5*c5+x6*c6
    pmaddubsw         m6, [rfilterq + %3q*8+48]   ;x7*c7+x8*c8
    paddw             m0, m2
    paddw             m4, m6
    paddw             m0, m4
%else
    pmaddwd           m0, [rfilterq + %3q*8   ]
    pmaddwd           m2, [rfilterq + %3q*8+16]
    pmaddwd           m4, [rfilterq + %3q*8+32]
    pmaddwd           m6, [rfilterq + %3q*8+48]
    paddd             m0, m2
    paddd             m4, m6
    paddd             m0, m4
    psrad             m0, %2-8
%if %1 > 4
    pmaddwd           m1, [rfilterq + %3q*8   ]
    pmaddwd           m3, [rfilterq + %3q*8+16]
    pmaddwd           m5, [rfilterq + %3q*8+32]
    pmaddwd           m7, [rfilterq + %3q*8+48]
    paddd             m1, m3
    paddd             m5, m7
    paddd             m1, m5
    psrad             m1, %2-8
%endif
    p%4               m0, m1
%endif
%endmacro

%macro QPEL_COMPUTE 2     ; width, bitdepth
%if %2 == 8
    pmaddubsw         m0, m12   ;x1*c1+x2*c2
    pmaddubsw         m2, m13   ;x3*c3+x4*c4
    pmaddubsw         m4, m14   ;x5*c5+x6*c6
    pmaddubsw         m6, m15   ;x7*c7+x8*c8
    paddw             m0, m2
    paddw             m4, m6
    paddw             m0, m4
%if %1 > 8
    pmaddubsw         m1, m12
    pmaddubsw         m3, m13
    pmaddubsw         m5, m14
    pmaddubsw         m7, m15
    paddw             m1, m3
    paddw             m5, m7
    paddw             m1, m5
%endif
%else
    pmaddwd           m0, m12
    pmaddwd           m2, m13
    pmaddwd           m4, m14
    pmaddwd           m6, m15
    paddd             m0, m2
    paddd             m4, m6
    paddd             m0, m4
    psrad             m0, %2-8
%if %1 > 4
    pmaddwd           m1, m12
    pmaddwd           m3, m13
    pmaddwd           m5, m14
    pmaddwd           m7, m15
    paddd             m1, m3
    paddd             m5, m7
    paddd             m1, m5
    psrad             m1, %2-8
%endif
%endif
%endmacro

%macro BI_COMPUTE 7     ; width, bitd, src1l, src1h, scr2l, scr2h, pw
    paddsw            %3, %5
%if %1 > 8
    paddsw            %4, %6
%endif
    UNI_COMPUTE       %1, %2, %3, %4, %7
%endmacro

%macro UNI_COMPUTE 5
    pmulhrsw          %3, %5
%if %1 > 8 || (%2 > 8 && %1 > 4)
    pmulhrsw          %4, %5
%endif
%if %2 == 8
    packuswb          %3, %4
%else
    pminsw            %3, [max_pixels_%2]
    pmaxsw            %3, [zero]
%if %1 > 8
    pminsw            %4, [max_pixels_%2]
    pmaxsw            %4, [zero]
%endif
%endif
%endmacro

INIT_XMM sse4                                    ; adds ff_ and _sse4 to function name
; ******************************
; void put_hevc_mc_pixels(int16_t *dst, ptrdiff_t dststride,
;                         uint8_t *_src, ptrdiff_t _srcstride,
;                         int height, int mx, int my)
; ******************************

%macro HEVC_PUT_HEVC_PEL_PIXELS 2
cglobal hevc_put_hevc_pel_pixels%1_%2, 5, 5, 3, dst, dststride, src, srcstride,height
    pxor               m2, m2
.loop
    SIMPLE_LOAD       %1, %2, srcq, m0
    MC_PIXEL_COMPUTE  %1, %2
    PEL_10STORE%1     dstq, m0, m1
    LOOP_END         dst, dststride, src, srcstride
    RET

cglobal hevc_put_hevc_uni_pel_pixels%1_%2, 5, 5, 3, dst, dststride, src, srcstride,height
    pxor              m2, m2
.loop
    SIMPLE_LOAD       %1, %2, srcq, m0
    PEL_%2STORE%1   dstq, m0, m1
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET

cglobal hevc_put_hevc_bi_pel_pixels%1_%2, 7, 7, 6, dst, dststride, src, srcstride, src2, src2stride,height
    pxor              m2, m2
    movdqa            m5, [pw_bi_%2]
.loop
    SIMPLE_LOAD       %1, %2, srcq, m0
    SIMPLE_BILOAD     %1, src2q, m3, m4
    MC_PIXEL_COMPUTE  %1, %2
    BI_COMPUTE        %1, %2, m0, m1, m3, m4, m5
    PEL_%2STORE%1   dstq, m0, m1
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    lea            src2q, [src2q+2*src2strideq]  ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET

%endmacro


; ******************************
; void put_hevc_epel_hX(int16_t *dst, ptrdiff_t dststride,
;                       uint8_t *_src, ptrdiff_t _srcstride,
;                       int width, int height, int mx, int my,
;                       int16_t* mcbuffer)
; ******************************


%macro HEVC_PUT_HEVC_EPEL 2
cglobal hevc_put_hevc_epel_h%1_%2, 6, 7, 15 , dst, dststride, src, srcstride, height, mx, rfilter
%assign %%stride ((%2 + 7)/8)
    EPEL_FILTER       %2, mx
.loop
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    PEL_10STORE%1      dstq, m0, m1
    LOOP_END         dst, dststride, src, srcstride
    RET

cglobal hevc_put_hevc_uni_epel_h%1_%2, 6, 7, 15 , dst, dststride, src, srcstride, height, mx, rfilter
%assign %%stride ((%2 + 7)/8)
    movdqa            m9, [pw_%2]
    EPEL_FILTER       %2, mx
.loop
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    UNI_COMPUTE       %1, %2, m0, m1, m9
    PEL_%2STORE%1   dstq, m0, m1
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET

cglobal hevc_put_hevc_bi_epel_h%1_%2, 8, 9, 15, dst, dststride, src, srcstride, src2, src2stride,height, mx, rfilter
    movdqa            m9, [pw_bi_%2]
    EPEL_FILTER       %2, mx
.loop
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SIMPLE_BILOAD     %1, src2q, m2, m3
    BI_COMPUTE        %1, %2, m0, m1, m2, m3, m9
    PEL_%2STORE%1   dstq, m0, m1
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    lea            src2q, [src2q+2*src2strideq]  ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET

; ******************************
; void put_hevc_epel_v(int16_t *dst, ptrdiff_t dststride,
;                      uint8_t *_src, ptrdiff_t _srcstride,
;                      int width, int height, int mx, int my,
;                      int16_t* mcbuffer)
; ******************************

cglobal hevc_put_hevc_epel_v%1_%2, 7, 8, 15 , dst, dststride, src, srcstride, height, r3src, my, rfilter
    lea           r3srcq, [srcstrideq*3]
    sub             srcq, srcstrideq
    EPEL_FILTER       %2, my
.loop
    EPEL_LOAD         %2, srcq, srcstride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    PEL_10STORE%1     dstq, m0, m1
    LOOP_END          dst, dststride, src, srcstride
    RET

cglobal hevc_put_hevc_uni_epel_v%1_%2, 7, 8, 15 , dst, dststride, src, srcstride, height, r3src, my, rfilter
    lea           r3srcq, [srcstrideq*3]
    movdqa            m9, [pw_%2]
    sub             srcq, srcstrideq
    EPEL_FILTER       %2, my
.loop
    EPEL_LOAD         %2, srcq, srcstride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    UNI_COMPUTE       %1, %2, m0, m1, m9
    PEL_%2STORE%1   dstq, m0, m1
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET


cglobal hevc_put_hevc_bi_epel_v%1_%2, 9, 10, 15, dst, dststride, src, srcstride, src2, src2stride,height, r3src, my, rfilter
    lea           r3srcq, [srcstrideq*3]
    movdqa            m9, [pw_bi_%2]
    sub             srcq, srcstrideq
    EPEL_FILTER       %2, my
.loop
    EPEL_LOAD         %2, srcq, srcstride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SIMPLE_BILOAD     %1, src2q, m2, m3
    BI_COMPUTE        %1, %2, m0, m1, m2, m3, m9
    PEL_%2STORE%1   dstq, m0, m1
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    lea            src2q, [src2q+2*src2strideq]  ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET
%endmacro


; ******************************
; void put_hevc_epel_hv(int16_t *dst, ptrdiff_t dststride,
;                       uint8_t *_src, ptrdiff_t _srcstride,
;                       int width, int height, int mx, int my)
; ******************************

%macro HEVC_PUT_HEVC_EPEL_HV 2
cglobal hevc_put_hevc_epel_hv%1_%2, 7, 9, 12 , dst, dststride, src, srcstride, height, mx, my, r3src, rfilter
%assign %%stride ((%2 + 7)/8)
    sub             srcq, srcstrideq
    EPEL_HV_FILTER    %2
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SWAP              m4, m0
    lea             srcq, [srcq + srcstrideq]
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SWAP              m5, m0
    lea             srcq, [srcq + srcstrideq]
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SWAP              m6, m0
    lea             srcq, [srcq + srcstrideq]
.loop
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SWAP              m7, m0
    punpcklwd         m0, m4, m5
    punpcklwd         m2, m6, m7
%if %1 > 4
    punpckhwd         m1, m4, m5
    punpckhwd         m3, m6, m7
%endif
    EPEL_COMPUTE      14, %1, m12, m13
    PEL_10STORE%1     dstq, m0, m1
    movdqa            m4, m5
    movdqa            m5, m6
    movdqa            m6, m7
    LOOP_END         dst, dststride, src, srcstride
    RET

cglobal hevc_put_hevc_uni_epel_hv%1_%2, 7, 9, 12 , dst, dststride, src, srcstride, height, mx, my, r3src, rfilter
%assign %%stride ((%2 + 7)/8)
    sub             srcq, srcstrideq
    EPEL_HV_FILTER    %2
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SWAP              m4, m0
    lea             srcq, [srcq + srcstrideq]
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SWAP              m5, m0
    lea             srcq, [srcq + srcstrideq]
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SWAP              m6, m0
    lea             srcq, [srcq + srcstrideq]
.loop
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SWAP              m7, m0
    punpcklwd         m0, m4, m5
    punpcklwd         m2, m6, m7
%if %1 > 4
    punpckhwd         m1, m4, m5
    punpckhwd         m3, m6, m7
%endif
    EPEL_COMPUTE      14, %1, m12, m13
    UNI_COMPUTE       %1, %2, m0, m1, [pw_%2]
    PEL_%2STORE%1   dstq, m0, m1
    movdqa            m4, m5
    movdqa            m5, m6
    movdqa            m6, m7
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET


cglobal hevc_put_hevc_bi_epel_hv%1_%2, 9, 11, 16, dst, dststride, src, srcstride, src2, src2stride, height, mx, my, r3src, rfilter
%assign %%stride ((%2 + 7)/8)
    sub             srcq, srcstrideq
    EPEL_HV_FILTER    %2
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SWAP              m4, m0
    lea             srcq, [srcq + srcstrideq]
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SWAP              m5, m0
    lea             srcq, [srcq + srcstrideq]
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SWAP              m6, m0
    lea             srcq, [srcq + srcstrideq]
.loop
    EPEL_LOAD         %2, srcq-%%stride, %%stride, %1
    EPEL_COMPUTE      %2, %1, m14, m15
    SWAP              m7, m0
    punpcklwd         m0, m4, m5
    punpcklwd         m2, m6, m7
%if %1 > 4
    punpckhwd         m1, m4, m5
    punpckhwd         m3, m6, m7
%endif
    EPEL_COMPUTE      14, %1, m12, m13
    SIMPLE_BILOAD     %1, src2q, m8, m9
    BI_COMPUTE        %1, %2, m0, m1, m8, m9, [pw_bi_%2]
    PEL_%2STORE%1   dstq, m0, m1
    movdqa            m4, m5
    movdqa            m5, m6
    movdqa            m6, m7
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    lea            src2q, [src2q+2*src2strideq]  ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET
%endmacro

; ******************************
; void put_hevc_qpel_hX_X_X(int16_t *dst, ptrdiff_t dststride,
;                       uint8_t *_src, ptrdiff_t _srcstride,
;                       int width, int height, int mx, int my)
; ******************************

%macro HEVC_PUT_HEVC_QPEL 2
cglobal hevc_put_hevc_qpel_h%1_%2, 6, 7, 15 , dst, dststride, src, srcstride, height, mx, rfilter
    QPEL_FILTER       %2, mx
.loop
    QPEL_H_LOAD       %2, srcq, %1, 10
    QPEL_COMPUTE      %1, %2
%if %2 > 8
    packssdw          m0, m1
%endif
    PEL_10STORE%1     dstq, m0, m1
    LOOP_END          dst, dststride, src, srcstride
    RET

cglobal hevc_put_hevc_uni_qpel_h%1_%2, 6, 7, 15 , dst, dststride, src, srcstride, height, mx, rfilter
    movdqa            m9, [pw_%2]
    QPEL_FILTER       %2, mx
.loop
    QPEL_H_LOAD       %2, srcq, %1, 10
    QPEL_COMPUTE      %1, %2
%if %2 > 8
    packssdw          m0, m1
%endif
    UNI_COMPUTE       %1, %2, m0, m1, m9
    PEL_%2STORE%1   dstq, m0, m1
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET

cglobal hevc_put_hevc_bi_qpel_h%1_%2, 8, 9, 16 , dst, dststride, src, srcstride, src2, src2stride, height, mx, rfilter
    movdqa            m9, [pw_bi_%2]
    QPEL_FILTER       %2, mx
.loop
    QPEL_H_LOAD       %2, srcq, %1, 10
    QPEL_COMPUTE      %1, %2
%if %2 > 8
    packssdw          m0, m1
%endif
    SIMPLE_BILOAD     %1, src2q, m10, m11
    BI_COMPUTE        %1, %2, m0, m1, m10, m11, m9
    PEL_%2STORE%1   dstq, m0, m1
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    lea            src2q, [src2q+2*src2strideq]  ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET


; ******************************
; void put_hevc_qpel_vX_X_X(int16_t *dst, ptrdiff_t dststride,
;                       uint8_t *_src, ptrdiff_t _srcstride,
;                       int width, int height, int mx, int my)
; ******************************

cglobal hevc_put_hevc_qpel_v%1_%2, 7, 14, 15 , dst, dststride, src, srcstride, height, r3src, my, rfilter
    lea           r3srcq, [srcstrideq*3]
    QPEL_FILTER       %2, my
.loop
    QPEL_V_LOAD       %2, srcq, srcstride, %1
    QPEL_COMPUTE      %1, %2
%if %2 > 8
    packssdw          m0, m1
%endif
    PEL_10STORE%1     dstq, m0, m1
    LOOP_END         dst, dststride, src, srcstride
    RET

cglobal hevc_put_hevc_uni_qpel_v%1_%2, 7, 14, 15 , dst, dststride, src, srcstride, height, r3src, my, rfilter
    movdqa            m9, [pw_%2]
    lea           r3srcq, [srcstrideq*3]
    QPEL_FILTER       %2, my
.loop
    QPEL_V_LOAD       %2, srcq, srcstride, %1
    QPEL_COMPUTE      %1, %2
%if %2 > 8
    packusdw          m0, m1
%endif
    UNI_COMPUTE       %1, %2, m0, m1, m9
    PEL_%2STORE%1   dstq, m0, m1
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET

cglobal hevc_put_hevc_bi_qpel_v%1_%2, 9, 14, 16 , dst, dststride, src, srcstride, src2, src2stride, height, r3src, my, rfilter
    movdqa            m9, [pw_bi_%2]
    lea           r3srcq, [srcstrideq*3]
    QPEL_FILTER       %2, my
.loop
    SIMPLE_BILOAD     %1, src2q, m10, m11
    QPEL_V_LOAD       %2, srcq, srcstride, %1
    QPEL_COMPUTE      %1, %2
%if %2 > 8
    packssdw          m0, m1
%endif
    BI_COMPUTE        %1, %2, m0, m1, m10, m11, m9
    PEL_%2STORE%1   dstq, m0, m1
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    lea            src2q, [src2q+2*src2strideq]  ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET
%endmacro


; ******************************
; void put_hevc_qpel_hvX_X(int16_t *dst, ptrdiff_t dststride,
;                       uint8_t *_src, ptrdiff_t _srcstride,
;                       int height, int mx, int my)
; ******************************
%macro HEVC_PUT_HEVC_QPEL_HV 2
cglobal hevc_put_hevc_qpel_hv%1_%2, 7, 9, 12 , dst, dststride, src, srcstride, height, mx, my, r3src, rfilter
    lea              mxq, [mxq*8-8]
    lea              myq, [myq*8-8]
    lea           r3srcq, [srcstrideq*3]
    sub             srcq, r3srcq
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP              m8, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP              m9, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m10, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m11, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m12, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m13, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m14, m0
    lea             srcq, [srcq + srcstrideq]
.loop
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m15, m0
    punpcklwd         m0, m8, m9
    punpcklwd         m2, m10, m11
    punpcklwd         m4, m12, m13
    punpcklwd         m6, m14, m15
%if %1 > 4
    punpckhwd         m1, m8, m9
    punpckhwd         m3, m10, m11
    punpckhwd         m5, m12, m13
    punpckhwd         m7, m14, m15
%endif
    QPEL_HV_COMPUTE   %1, 14, my, ackssdw
    PEL_10STORE%1     dstq, m0, m1
%if %1 <= 4
    movq              m8, m9
    movq              m9, m10
    movq             m10, m11
    movq             m11, m12
    movq             m12, m13
    movq             m13, m14
    movq             m14, m15
%else
    movdqa            m8, m9
    movdqa            m9, m10
    movdqa           m10, m11
    movdqa           m11, m12
    movdqa           m12, m13
    movdqa           m13, m14
    movdqa           m14, m15
%endif
    LOOP_END         dst, dststride, src, srcstride
    RET

cglobal hevc_put_hevc_uni_qpel_hv%1_%2, 7, 9, 12 , dst, dststride, src, srcstride, height, mx, my, r3src, rfilter
    lea              mxq, [mxq*8-8]
    lea              myq, [myq*8-8]
    lea           r3srcq, [srcstrideq*3]
    sub             srcq, r3srcq
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP              m8, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP              m9, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m10, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m11, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m12, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m13, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m14, m0
    lea             srcq, [srcq + srcstrideq]
.loop
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m15, m0
    punpcklwd         m0, m8, m9
    punpcklwd         m2, m10, m11
    punpcklwd         m4, m12, m13
    punpcklwd         m6, m14, m15
%if %1 > 4
    punpckhwd         m1, m8, m9
    punpckhwd         m3, m10, m11
    punpckhwd         m5, m12, m13
    punpckhwd         m7, m14, m15
%endif
    QPEL_HV_COMPUTE   %1, 14, my, ackusdw
    UNI_COMPUTE       %1, %2, m0, m1, [pw_%2]
    PEL_%2STORE%1   dstq, m0, m1

%if %1 <= 4
    movq              m8, m9
    movq              m9, m10
    movq             m10, m11
    movq             m11, m12
    movq             m12, m13
    movq             m13, m14
    movq             m14, m15
%else
    movdqa            m8, m9
    movdqa            m9, m10
    movdqa           m10, m11
    movdqa           m11, m12
    movdqa           m12, m13
    movdqa           m13, m14
    movdqa           m14, m15
%endif
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET

cglobal hevc_put_hevc_bi_qpel_hv%1_%2, 9, 11, 16, dst, dststride, src, srcstride, src2, src2stride, height, mx, my, r3src, rfilter
    lea              mxq, [mxq*8-8]
    lea              myq, [myq*8-8]
    lea           r3srcq, [srcstrideq*3]
    sub             srcq, r3srcq
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP              m8, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP              m9, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m10, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m11, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m12, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m13, m0
    lea             srcq, [srcq + srcstrideq]
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m14, m0
    lea             srcq, [srcq + srcstrideq]
.loop
    QPEL_H_LOAD       %2, srcq, %1, 15
    QPEL_HV_COMPUTE   %1, %2, mx, ackssdw
    SWAP             m15, m0
    punpcklwd         m0, m8, m9
    punpcklwd         m2, m10, m11
    punpcklwd         m4, m12, m13
    punpcklwd         m6, m14, m15
%if %1 > 4
    punpckhwd         m1, m8, m9
    punpckhwd         m3, m10, m11
    punpckhwd         m5, m12, m13
    punpckhwd         m7, m14, m15
%endif
    QPEL_HV_COMPUTE   %1, 14, my, ackssdw
    SIMPLE_BILOAD     %1, src2q, m8, m9 ;m9 not used in this case
    BI_COMPUTE        %1, %2, m0, m1, m8, m9, [pw_bi_%2]
    PEL_%2STORE%1   dstq, m0, m1

%if %1 <= 4
    movq              m8, m9
    movq              m9, m10
    movq             m10, m11
    movq             m11, m12
    movq             m12, m13
    movq             m13, m14
    movq             m14, m15
%else
    movdqa            m8, m9
    movdqa            m9, m10
    movdqa           m10, m11
    movdqa           m11, m12
    movdqa           m12, m13
    movdqa           m13, m14
    movdqa           m14, m15
%endif
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+srcstrideq]      ; src += srcstride
    lea            src2q, [src2q+2*src2strideq]  ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET
%endmacro

%macro WEIGHTING_FUNCS 2
cglobal hevc_put_hevc_uni_w%1_%2, 8, 10, 11, dst, dststride, src, srcstride, height, denom, wx, ox, shift
    lea          shiftd, [denomd+14-%2]          ; shift = 14 - bitd + denom
    shl             oxd, %2-8                    ; ox << (bitd - 8)
    movd             m2, wxd        ; WX
    movd             m3, oxd        ; OX
    movd             m4, shiftd     ; shift
    punpcklwd        m2, m2
    pshufd           m3, m3, 0
    pshufd           m2, m2, 0
    sub          shiftd, 1
    movd             m6, shiftd
    movdqu           m5, [one_per_32]
    pslld            m5, m6
.loop
   SIMPLE_LOAD        %1, 10, srcq, m0
    pmulhw            m6, m0, m2
    pmullw            m0, m2
    punpckhwd         m1, m0, m6
    punpcklwd         m0, m6
    paddd             m0, m5
    paddd             m1, m5
    psrad             m0, m4
    psrad             m1, m4
    paddd             m0, m3
    paddd             m1, m3
    packusdw          m0, m1
%if %2 == 8
    packuswb          m0, m0
%else
    pminsw            m0, [max_pixels_%2]
%endif
    PEL_%2STORE%1   dstq, m0, m1
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+2*srcstrideq]      ; src += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET

cglobal hevc_put_hevc_bi_w%1_%2, 12, 14, 14, dst, dststride, src, srcstride, src2, src2stride, height, denom, wx0, wx1, ox0, ox1, shift, temp
    shl             ox0d, %2-8                    ; ox << (bitd - 8)
    shl             ox1d, %2-8                    ; ox << (bitd - 8)
    lea           shiftd, [denomd+14-%2]          ; shift = 14 - bitd + denom
    movd              m2, wx0d        ; WX0
    movd              m3, wx1d        ; WX1
    punpcklwd         m2, m2
    punpcklwd         m3, m3
    pshufd            m2, m2, 0
    pshufd            m3, m3, 0
    add             ox0d, ox1d
    add             ox0d, 1
    movd              m4, ox0d         ; offset
    pshufd            m4, m4, 0
    movd              m5, shiftd       ; shift
    pslld             m4, m5
    add           shiftd, 1
    movd              m5, shiftd       ; shift

.loop
   SIMPLE_LOAD        %1, 10, srcq,  m0
   SIMPLE_LOAD        %1, 10, src2q, m10
    pmulhw            m6, m0, m3
    pmullw            m0, m3
    pmulhw            m7, m10, m2
    pmullw           m10, m2
    punpckhwd         m1, m0, m6
    punpcklwd         m0, m6
    punpckhwd        m11, m10, m7
    punpcklwd        m10, m7
    paddd             m0, m10
    paddd             m1, m11
    paddd             m0, m4
    paddd             m1, m4
    psrad             m0, m5
    psrad             m1, m5
    packusdw          m0, m1
%if %2 == 8
    packuswb          m0, m0
%else
    pminsw            m0, [max_pixels_%2]
%endif
    PEL_%2STORE%1   dstq, m0, m1
    lea             dstq, [dstq+dststrideq]      ; dst += dststride
    lea             srcq, [srcq+2*srcstrideq]      ; src += srcstride
    lea            src2q, [src2q+2*src2strideq]      ; src2 += srcstride
    dec          heightd                         ; cmp height
    jnz               .loop                      ; height loop
    RET
%endmacro

WEIGHTING_FUNCS 2, 8
WEIGHTING_FUNCS 4, 8
WEIGHTING_FUNCS 6, 8
WEIGHTING_FUNCS 8, 8

WEIGHTING_FUNCS 2, 10
WEIGHTING_FUNCS 4, 10
WEIGHTING_FUNCS 6, 10
WEIGHTING_FUNCS 8, 10

HEVC_PUT_HEVC_PEL_PIXELS  2, 8
HEVC_PUT_HEVC_PEL_PIXELS  4, 8
HEVC_PUT_HEVC_PEL_PIXELS  6, 8
HEVC_PUT_HEVC_PEL_PIXELS  8, 8
HEVC_PUT_HEVC_PEL_PIXELS 12, 8
HEVC_PUT_HEVC_PEL_PIXELS 16, 8

HEVC_PUT_HEVC_PEL_PIXELS 2, 10
HEVC_PUT_HEVC_PEL_PIXELS 4, 10
HEVC_PUT_HEVC_PEL_PIXELS 6, 10
HEVC_PUT_HEVC_PEL_PIXELS 8, 10


HEVC_PUT_HEVC_EPEL 2,  8
HEVC_PUT_HEVC_EPEL 4,  8
HEVC_PUT_HEVC_EPEL 6,  8
HEVC_PUT_HEVC_EPEL 8,  8
HEVC_PUT_HEVC_EPEL 12, 8
HEVC_PUT_HEVC_EPEL 16, 8


HEVC_PUT_HEVC_EPEL 2, 10
HEVC_PUT_HEVC_EPEL 4, 10
HEVC_PUT_HEVC_EPEL 6, 10
HEVC_PUT_HEVC_EPEL 8, 10


HEVC_PUT_HEVC_EPEL_HV 2,  8
HEVC_PUT_HEVC_EPEL_HV 4,  8
HEVC_PUT_HEVC_EPEL_HV 6,  8
HEVC_PUT_HEVC_EPEL_HV 8,  8

HEVC_PUT_HEVC_EPEL_HV 2, 10
HEVC_PUT_HEVC_EPEL_HV 4, 10
HEVC_PUT_HEVC_EPEL_HV 6, 10
HEVC_PUT_HEVC_EPEL_HV 8, 10


HEVC_PUT_HEVC_QPEL 4,  8
HEVC_PUT_HEVC_QPEL 8,  8
HEVC_PUT_HEVC_QPEL 12, 8
HEVC_PUT_HEVC_QPEL 16, 8

HEVC_PUT_HEVC_QPEL 4, 10
HEVC_PUT_HEVC_QPEL 8, 10

HEVC_PUT_HEVC_QPEL_HV 2, 8
HEVC_PUT_HEVC_QPEL_HV 4, 8
HEVC_PUT_HEVC_QPEL_HV 6, 8
HEVC_PUT_HEVC_QPEL_HV 8, 8

HEVC_PUT_HEVC_QPEL_HV 2, 10
HEVC_PUT_HEVC_QPEL_HV 4, 10
HEVC_PUT_HEVC_QPEL_HV 6, 10
HEVC_PUT_HEVC_QPEL_HV 8, 10

%endif ; ARCH_X86_64
