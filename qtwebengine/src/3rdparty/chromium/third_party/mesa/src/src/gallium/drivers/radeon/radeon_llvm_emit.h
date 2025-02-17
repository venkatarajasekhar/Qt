/*
 * Copyright 2012 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors: Tom Stellard <thomas.stellard@amd.com>
 *
 */

#ifndef RADEON_LLVM_EMIT_H
#define RADEON_LLVM_EMIT_H

#include <llvm-c/Core.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned radeon_llvm_bitcode_compile(
   unsigned char * bitcode, unsigned bitcode_len,
   unsigned char ** bytes, unsigned * byte_count,
   const  char * gpu_family, unsigned dump);

unsigned  radeon_llvm_compile(
	LLVMModuleRef M,
	unsigned char ** bytes,
	unsigned * byte_count,
	const char * gpu_family,
	unsigned dump);

#ifdef __cplusplus
} /* Extern "C" */
#endif

#endif /* RADEON_LLVM_EMIT_H */
