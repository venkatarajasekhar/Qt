// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "crypto/ghash.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace crypto {

namespace {

// Test vectors are taken from Appendix B of
// http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/gcm/gcm-revised-spec.pdf

static const uint8 kKey1[16] = {
  0x66, 0xe9, 0x4b, 0xd4, 0xef, 0x8a, 0x2c, 0x3b,
  0x88, 0x4c, 0xfa, 0x59, 0xca, 0x34, 0x2b, 0x2e,
};

static const uint8 kCiphertext2[] = {
  0x03, 0x88, 0xda, 0xce, 0x60, 0xb6, 0xa3, 0x92,
  0xf3, 0x28, 0xc2, 0xb9, 0x71, 0xb2, 0xfe, 0x78,
};

static const uint8 kKey3[16] = {
  0xb8, 0x3b, 0x53, 0x37, 0x08, 0xbf, 0x53, 0x5d,
  0x0a, 0xa6, 0xe5, 0x29, 0x80, 0xd5, 0x3b, 0x78,
};

static const uint8 kCiphertext3[] = {
  0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24,
  0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c,
  0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0,
  0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e,
  0x21, 0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c,
  0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05,
  0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97,
  0x3d, 0x58, 0xe0, 0x91, 0x47, 0x3f, 0x59, 0x85,
};

static const uint8 kAdditional4[] = {
  0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
  0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
  0xab, 0xad, 0xda, 0xd2,
};

struct TestCase {
  const uint8* key;
  const uint8* additional;
  unsigned additional_length;
  const uint8* ciphertext;
  unsigned ciphertext_length;
  const uint8 expected[16];
};

static const TestCase kTestCases[] = {
  {
    kKey1,
    NULL,
    0,
    NULL,
    0,
    {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
  },
  {
    kKey1,
    NULL,
    0,
    kCiphertext2,
    sizeof(kCiphertext2),
    {
      0xf3, 0x8c, 0xbb, 0x1a, 0xd6, 0x92, 0x23, 0xdc,
      0xc3, 0x45, 0x7a, 0xe5, 0xb6, 0xb0, 0xf8, 0x85,
    },
  },
  {
    kKey3,
    NULL,
    0,
    kCiphertext3,
    sizeof(kCiphertext3),
    {
      0x7f, 0x1b, 0x32, 0xb8, 0x1b, 0x82, 0x0d, 0x02,
      0x61, 0x4f, 0x88, 0x95, 0xac, 0x1d, 0x4e, 0xac,
    },
  },
  {
    kKey3,
    kAdditional4,
    sizeof(kAdditional4),
    kCiphertext3,
    sizeof(kCiphertext3) - 4,
    {
      0x69, 0x8e, 0x57, 0xf7, 0x0e, 0x6e, 0xcc, 0x7f,
      0xd9, 0x46, 0x3b, 0x72, 0x60, 0xa9, 0xae, 0x5f,
    },
  },
};

TEST(GaloisHash, TestCases) {
  uint8 out[16];

  for (size_t i = 0; i < arraysize(kTestCases); ++i) {
    const TestCase& test = kTestCases[i];

    GaloisHash hash(test.key);
    if (test.additional_length)
      hash.UpdateAdditional(test.additional, test.additional_length);
    if (test.ciphertext_length)
      hash.UpdateCiphertext(test.ciphertext, test.ciphertext_length);
    hash.Finish(out, sizeof(out));
    EXPECT_TRUE(0 == memcmp(out, test.expected, 16));
  }
}

TEST(GaloisHash, VaryLengths) {
  uint8 out[16];

  for (size_t chunk_size = 1; chunk_size < 16; chunk_size++) {
    for (size_t i = 0; i < arraysize(kTestCases); ++i) {
      const TestCase& test = kTestCases[i];

      GaloisHash hash(test.key);
      for (size_t i = 0; i < test.additional_length;) {
        size_t n = std::min(test.additional_length - i, chunk_size);
        hash.UpdateAdditional(test.additional + i, n);
        i += n;
      }
      for (size_t i = 0; i < test.ciphertext_length;) {
        size_t n = std::min(test.ciphertext_length - i, chunk_size);
        hash.UpdateCiphertext(test.ciphertext + i, n);
        i += n;
      }
      hash.Finish(out, sizeof(out));
      EXPECT_TRUE(0 == memcmp(out, test.expected, 16));
    }
  }
}

}  // namespace

}  // namespace crypto
