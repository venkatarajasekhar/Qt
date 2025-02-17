/*
 * libjingle
 * Copyright 2012 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "talk/base/basictypes.h"

#include "talk/base/gunit.h"

namespace talk_base {

TEST(BasicTypesTest, Endian) {
  uint16 v16 = 0x1234u;
  uint8 first_byte = *reinterpret_cast<uint8*>(&v16);
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(0x34u, first_byte);
#elif defined(ARCH_CPU_BIG_ENDIAN)
  EXPECT_EQ(0x12u, first_byte);
#endif
}

TEST(BasicTypesTest, SizeOfTypes) {
  int8 i8 = -1;
  uint8 u8 = 1u;
  int16 i16 = -1;
  uint16 u16 = 1u;
  int32 i32 = -1;
  uint32 u32 = 1u;
  int64 i64 = -1;
  uint64 u64 = 1u;
  EXPECT_EQ(1u, sizeof(i8));
  EXPECT_EQ(1u, sizeof(u8));
  EXPECT_EQ(2u, sizeof(i16));
  EXPECT_EQ(2u, sizeof(u16));
  EXPECT_EQ(4u, sizeof(i32));
  EXPECT_EQ(4u, sizeof(u32));
  EXPECT_EQ(8u, sizeof(i64));
  EXPECT_EQ(8u, sizeof(u64));
  EXPECT_GT(0, i8);
  EXPECT_LT(0u, u8);
  EXPECT_GT(0, i16);
  EXPECT_LT(0u, u16);
  EXPECT_GT(0, i32);
  EXPECT_LT(0u, u32);
  EXPECT_GT(0, i64);
  EXPECT_LT(0u, u64);
}

TEST(BasicTypesTest, SizeOfConstants) {
  EXPECT_EQ(8u, sizeof(INT64_C(0)));
  EXPECT_EQ(8u, sizeof(UINT64_C(0)));
  EXPECT_EQ(8u, sizeof(INT64_C(0x1234567887654321)));
  EXPECT_EQ(8u, sizeof(UINT64_C(0x8765432112345678)));
}

// Test CPU_ macros
#if !defined(CPU_ARM) && defined(__arm__)
#error expected CPU_ARM to be defined.
#endif
#if !defined(CPU_X86) && (defined(WIN32) || defined(OSX))
#error expected CPU_X86 to be defined.
#endif
#if !defined(ARCH_CPU_LITTLE_ENDIAN) && \
  (defined(WIN32) || defined(OSX) || defined(CPU_X86))
#error expected ARCH_CPU_LITTLE_ENDIAN to be defined.
#endif

// TODO(fbarchard): Test all macros in basictypes.h

}  // namespace talk_base
