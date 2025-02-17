;
; jsimdext.inc - common declarations
;
; Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
; Copyright 2010 D. R. Commander
;
; Based on
; x86 SIMD extension for IJG JPEG library - version 1.02
;
; Copyright (C) 1999-2006, MIYASAKA Masaru.
;
; This software is provided 'as-is', without any express or implied
; warranty.  In no event will the authors be held liable for any damages
; arising from the use of this software.
;
; Permission is granted to anyone to use this software for any purpose,
; including commercial applications, and to alter it and redistribute it
; freely, subject to the following restrictions:
;
; 1. The origin of this software must not be misrepresented; you must not
;    claim that you wrote the original software. If you use this software
;    in a product, an acknowledgment in the product documentation would be
;    appreciated but is not required.
; 2. Altered source versions must be plainly marked as such, and must not be
;    misrepresented as being the original software.
; 3. This notice may not be removed or altered from any source distribution.
;
; [TAB8]

; ==========================================================================
;  System-dependent configurations

%ifdef WIN32	; ----(nasm -fwin32 -DWIN32 ...)--------
; * Microsoft Visual C++
; * MinGW (Minimalist GNU for Windows)
; * CygWin
; * LCC-Win32

; -- segment definition --
;
%ifdef __YASM_VER__
%define SEG_TEXT    .text  align=16
%define SEG_CONST   .rdata align=16
%else
%define SEG_TEXT    .text  align=16 public use32 class=CODE
%define SEG_CONST   .rdata align=16 public use32 class=CONST
%endif

%elifdef WIN64	; ----(nasm -fwin64 -DWIN64 ...)--------
; * Microsoft Visual C++

; -- segment definition --
;
%ifdef __YASM_VER__
%define SEG_TEXT    .text  align=16
%define SEG_CONST   .rdata align=16
%else
%define SEG_TEXT    .text  align=16 public use64 class=CODE
%define SEG_CONST   .rdata align=16 public use64 class=CONST
%endif
%define EXTN(name)  name			; foo() -> foo

%elifdef OBJ32	; ----(nasm -fobj -DOBJ32 ...)----------
; * Borland C++ (Win32)

; -- segment definition --
;
%define SEG_TEXT    .text  align=16 public use32 class=CODE
%define SEG_CONST   .data  align=16 public use32 class=DATA

%elifdef ELF	; ----(nasm -felf[64] -DELF ...)------------
; * Linux
; * *BSD family Unix using elf format
; * Unix System V, including Solaris x86, UnixWare and SCO Unix

; PIC is the default on Linux
%define PIC

; mark stack as non-executable
section .note.GNU-stack noalloc noexec nowrite progbits

; -- segment definition --
;
%ifdef __x86_64__
%define SEG_TEXT    .text   progbits align=16
%define SEG_CONST   .rodata progbits align=16
%else
%define SEG_TEXT    .text   progbits alloc exec   nowrite align=16
%define SEG_CONST   .rodata progbits alloc noexec nowrite align=16
%endif

; To make the code position-independent, append -DPIC to the commandline
;
%define GOT_SYMBOL  _GLOBAL_OFFSET_TABLE_	; ELF supports PIC
%define EXTN(name)  name			; foo() -> foo

%elifdef AOUT	; ----(nasm -faoutb/aout -DAOUT ...)----
; * Older Linux using a.out format  (nasm -f aout -DAOUT ...)
; * *BSD family Unix using a.out format  (nasm -f aoutb -DAOUT ...)

; -- segment definition --
;
%define SEG_TEXT    .text
%define SEG_CONST   .data

; To make the code position-independent, append -DPIC to the commandline
;
%define GOT_SYMBOL  __GLOBAL_OFFSET_TABLE_	; BSD-style a.out supports PIC

%elifdef MACHO	; ----(nasm -fmacho -DMACHO ...)--------
; * NeXTstep/OpenStep/Rhapsody/Darwin/MacOS X (Mach-O format)

; -- segment definition --
;
%define SEG_TEXT    .text  ;align=16	; nasm doesn't accept align=16. why?
%define SEG_CONST   .rodata align=16

; The generation of position-independent code (PIC) is the default on Darwin.
;
%define PIC
%define GOT_SYMBOL  _MACHO_PIC_		; Mach-O style code-relative addressing

%else		; ----(Other case)----------------------

; -- segment definition --
;
%define SEG_TEXT    .text
%define SEG_CONST   .data

%endif	; ----------------------------------------------

; ==========================================================================

; --------------------------------------------------------------------------
;  Common types
;
%ifdef __x86_64__
%define POINTER                 qword           ; general pointer type
%define SIZEOF_POINTER          SIZEOF_QWORD    ; sizeof(POINTER)
%define POINTER_BIT             QWORD_BIT       ; sizeof(POINTER)*BYTE_BIT
%else
%define POINTER                 dword           ; general pointer type
%define SIZEOF_POINTER          SIZEOF_DWORD    ; sizeof(POINTER)
%define POINTER_BIT             DWORD_BIT       ; sizeof(POINTER)*BYTE_BIT
%endif

%define INT                     dword           ; signed integer type
%define SIZEOF_INT              SIZEOF_DWORD    ; sizeof(INT)
%define INT_BIT                 DWORD_BIT       ; sizeof(INT)*BYTE_BIT

%define FP32                    dword           ; IEEE754 single
%define SIZEOF_FP32             SIZEOF_DWORD    ; sizeof(FP32)
%define FP32_BIT                DWORD_BIT       ; sizeof(FP32)*BYTE_BIT

%define MMWORD                  qword           ; int64  (MMX register)
%define SIZEOF_MMWORD           SIZEOF_QWORD    ; sizeof(MMWORD)
%define MMWORD_BIT              QWORD_BIT       ; sizeof(MMWORD)*BYTE_BIT

; NASM is buggy and doesn't properly handle operand sizes for SSE
; instructions, so for now we have to define XMMWORD as blank.
%define XMMWORD                                 ; int128 (SSE register)
%define SIZEOF_XMMWORD          SIZEOF_OWORD    ; sizeof(XMMWORD)
%define XMMWORD_BIT             OWORD_BIT       ; sizeof(XMMWORD)*BYTE_BIT

; Similar hacks for when we load a dword or MMWORD into an xmm# register
%define XMM_DWORD
%define XMM_MMWORD

%define SIZEOF_BYTE             1               ; sizeof(BYTE)
%define SIZEOF_WORD             2               ; sizeof(WORD)
%define SIZEOF_DWORD            4               ; sizeof(DWORD)
%define SIZEOF_QWORD            8               ; sizeof(QWORD)
%define SIZEOF_OWORD            16              ; sizeof(OWORD)

%define BYTE_BIT                8               ; CHAR_BIT in C
%define WORD_BIT                16              ; sizeof(WORD)*BYTE_BIT
%define DWORD_BIT               32              ; sizeof(DWORD)*BYTE_BIT
%define QWORD_BIT               64              ; sizeof(QWORD)*BYTE_BIT
%define OWORD_BIT               128             ; sizeof(OWORD)*BYTE_BIT

; --------------------------------------------------------------------------
;  External Symbol Name
;
%ifndef EXTN
%define EXTN(name)   _ %+ name		; foo() -> _foo
%endif

; --------------------------------------------------------------------------
;  Macros for position-independent code (PIC) support
;
%ifndef GOT_SYMBOL
%undef PIC
%endif

%ifdef PIC ; -------------------------------------------

%ifidn GOT_SYMBOL,_MACHO_PIC_ ; --------------------

; At present, nasm doesn't seem to support PIC generation for Mach-O.
; The PIC support code below is a little tricky.

	SECTION	SEG_CONST
const_base:

%define GOTOFF(got,sym) (got) + (sym) - const_base

%imacro get_GOT	1
	; NOTE: this macro destroys ecx resister.
	call	%%geteip
	add	ecx, byte (%%ref - $)
	jmp	short %%adjust
%%geteip:
	mov	ecx, POINTER [esp]
	ret
%%adjust:
	push	ebp
	xor	ebp,ebp		; ebp = 0
%ifidni %1,ebx	; (%1 == ebx)
	; db 0x8D,0x9C + jmp near const_base =
	;   lea ebx, [ecx+ebp*8+(const_base-%%ref)] ; 8D,9C,E9,(offset32)
	db	0x8D,0x9C		; 8D,9C
	jmp	near const_base		; E9,(const_base-%%ref)
%%ref:
%else  ; (%1 != ebx)
	; db 0x8D,0x8C + jmp near const_base =
	;   lea ecx, [ecx+ebp*8+(const_base-%%ref)] ; 8D,8C,E9,(offset32)
	db	0x8D,0x8C		; 8D,8C
	jmp	near const_base		; E9,(const_base-%%ref)
%%ref:	mov	%1, ecx
%endif ; (%1 == ebx)
	pop	ebp
%endmacro

%else	; GOT_SYMBOL != _MACHO_PIC_ ----------------

%define GOTOFF(got,sym) (got) + (sym) wrt ..gotoff

%imacro get_GOT	1
	extern	GOT_SYMBOL
	call	%%geteip
	add	%1, GOT_SYMBOL + $$ - $ wrt ..gotpc
	jmp	short %%done
%%geteip:
	mov	%1, POINTER [esp]
	ret
%%done:
%endmacro

%endif	; GOT_SYMBOL == _MACHO_PIC_ ----------------

%imacro pushpic	1.nolist
	push	%1
%endmacro
%imacro poppic	1.nolist
	pop	%1
%endmacro
%imacro movpic	2.nolist
	mov	%1,%2
%endmacro

%else	; !PIC -----------------------------------------

%define GOTOFF(got,sym) (sym)

%imacro get_GOT	1.nolist
%endmacro
%imacro pushpic	1.nolist
%endmacro
%imacro poppic	1.nolist
%endmacro
%imacro movpic	2.nolist
%endmacro

%endif	;  PIC -----------------------------------------

; --------------------------------------------------------------------------
;  Align the next instruction on {2,4,8,16,..}-byte boundary.
;  ".balign n,,m" in GNU as
;
%define MSKLE(x,y)  (~(((y) & 0xFFFF) - ((x) & 0xFFFF)) >> 16)
%define FILLB(b,n)  (($$-(b)) & ((n)-1))

%imacro alignx 1-2.nolist 0xFFFF
%%bs:	times MSKLE(FILLB(%%bs,%1),%2) & MSKLE(16,FILLB($,%1)) & FILLB($,%1) \
	       db 0x90                               ; nop
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/9 \
	       db 0x8D,0x9C,0x23,0x00,0x00,0x00,0x00 ; lea ebx,[ebx+0x00000000]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/7 \
	       db 0x8D,0xAC,0x25,0x00,0x00,0x00,0x00 ; lea ebp,[ebp+0x00000000]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/6 \
	       db 0x8D,0xAD,0x00,0x00,0x00,0x00      ; lea ebp,[ebp+0x00000000]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/4 \
	       db 0x8D,0x6C,0x25,0x00                ; lea ebp,[ebp+0x00]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/3 \
	       db 0x8D,0x6D,0x00                     ; lea ebp,[ebp+0x00]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/2 \
	       db 0x8B,0xED                          ; mov ebp,ebp
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/1 \
	       db 0x90                               ; nop
%endmacro

; Align the next data on {2,4,8,16,..}-byte boundary.
;
%imacro alignz 1.nolist
	align %1, db 0		; filling zeros
%endmacro

%ifdef __x86_64__

%ifdef WIN64

%imacro collect_args 0
	push r12
	push r13
	push r14
	push r15
	mov r10, rcx
	mov r11, rdx
	mov r12, r8
	mov r13, r9
	mov r14, [rax+48]
	mov r15, [rax+56]
	push rsi
	push rdi
	sub     rsp, SIZEOF_XMMWORD
	movaps  XMMWORD [rsp], xmm6
	sub     rsp, SIZEOF_XMMWORD
	movaps  XMMWORD [rsp], xmm7
%endmacro

%imacro uncollect_args 0
	movaps  xmm7, XMMWORD [rsp]
	add     rsp, SIZEOF_XMMWORD
	movaps  xmm6, XMMWORD [rsp]
	add     rsp, SIZEOF_XMMWORD
	pop rdi
	pop rsi
	pop r15
	pop r14
	pop r13
	pop r12
%endmacro

%else

%imacro collect_args 0
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	mov r10, rdi
	mov r11, rsi
	mov r12, rdx
	mov r13, rcx
	mov r14, r8
	mov r15, r9
%endmacro

%imacro uncollect_args 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
%endmacro

%endif

%endif

; --------------------------------------------------------------------------
;  Defines picked up from the C headers
;
%include "jsimdcfg.inc"

; Begin chromium edits
%ifdef MACHO ; ----(nasm -fmacho -DMACHO ...)--------
%define PRIVATE :private_extern
%elifdef ELF ; ----(nasm -felf[64] -DELF ...)------------
%define PRIVATE :hidden
%else
%define PRIVATE
%endif
; End chromium edits

; --------------------------------------------------------------------------
