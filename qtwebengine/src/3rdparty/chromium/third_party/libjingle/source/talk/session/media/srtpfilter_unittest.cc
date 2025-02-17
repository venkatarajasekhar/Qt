/*
 * libjingle
 * Copyright 2004 Google Inc.
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

#include "talk/base/byteorder.h"
#include "talk/base/gunit.h"
#include "talk/base/thread.h"
#include "talk/media/base/cryptoparams.h"
#include "talk/media/base/fakertp.h"
#include "talk/p2p/base/sessiondescription.h"
#include "talk/session/media/srtpfilter.h"
#ifdef SRTP_RELATIVE_PATH
#include "crypto/include/err.h"
#else
#include "third_party/libsrtp/crypto/include/err.h"
#endif

using cricket::CS_AES_CM_128_HMAC_SHA1_80;
using cricket::CS_AES_CM_128_HMAC_SHA1_32;
using cricket::CryptoParams;
using cricket::CS_LOCAL;
using cricket::CS_REMOTE;

static const uint8 kTestKey1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234";
static const uint8 kTestKey2[] = "4321ZYXWVUTSRQPONMLKJIHGFEDCBA";
static const int kTestKeyLen = 30;
static const std::string kTestKeyParams1 =
    "inline:WVNfX19zZW1jdGwgKCkgewkyMjA7fQp9CnVubGVz";
static const std::string kTestKeyParams2 =
    "inline:PS1uQCVeeCFCanVmcjkpPywjNWhcYD0mXXtxaVBR";
static const std::string kTestKeyParams3 =
    "inline:1234X19zZW1jdGwgKCkgewkyMjA7fQp9CnVubGVz";
static const std::string kTestKeyParams4 =
    "inline:4567QCVeeCFCanVmcjkpPywjNWhcYD0mXXtxaVBR";
static const cricket::CryptoParams kTestCryptoParams1(
    1, "AES_CM_128_HMAC_SHA1_80", kTestKeyParams1, "");
static const cricket::CryptoParams kTestCryptoParams2(
    1, "AES_CM_128_HMAC_SHA1_80", kTestKeyParams2, "");

static int rtp_auth_tag_len(const std::string& cs) {
  return (cs == CS_AES_CM_128_HMAC_SHA1_32) ? 4 : 10;
}
static int rtcp_auth_tag_len(const std::string& cs) {
  return 10;
}

class SrtpFilterTest : public testing::Test {
 protected:
  SrtpFilterTest()
  // Need to initialize |sequence_number_|, the value does not matter.
      : sequence_number_(1) {
  }
  static std::vector<CryptoParams> MakeVector(const CryptoParams& params) {
    std::vector<CryptoParams> vec;
    vec.push_back(params);
    return vec;
  }
  void TestSetParams(const std::vector<CryptoParams>& params1,
                     const std::vector<CryptoParams>& params2) {
    EXPECT_TRUE(f1_.SetOffer(params1, CS_LOCAL));
    EXPECT_TRUE(f2_.SetOffer(params1, CS_REMOTE));
    EXPECT_TRUE(f2_.SetAnswer(params2, CS_LOCAL));
    EXPECT_TRUE(f1_.SetAnswer(params2, CS_REMOTE));
    EXPECT_TRUE(f1_.IsActive());
  }
  void TestProtectUnprotect(const std::string& cs1, const std::string& cs2) {
    char rtp_packet[sizeof(kPcmuFrame) + 10];
    char original_rtp_packet[sizeof(kPcmuFrame)];
    char rtcp_packet[sizeof(kRtcpReport) + 4 + 10];
    int rtp_len = sizeof(kPcmuFrame), rtcp_len = sizeof(kRtcpReport), out_len;
    memcpy(rtp_packet, kPcmuFrame, rtp_len);
    // In order to be able to run this test function multiple times we can not
    // use the same sequence number twice. Increase the sequence number by one.
    talk_base::SetBE16(reinterpret_cast<uint8*>(rtp_packet) + 2,
                       ++sequence_number_);
    memcpy(original_rtp_packet, rtp_packet, rtp_len);
    memcpy(rtcp_packet, kRtcpReport, rtcp_len);

    EXPECT_TRUE(f1_.ProtectRtp(rtp_packet, rtp_len,
                               sizeof(rtp_packet), &out_len));
    EXPECT_EQ(out_len, rtp_len + rtp_auth_tag_len(cs1));
    EXPECT_NE(0, memcmp(rtp_packet, original_rtp_packet, rtp_len));
    EXPECT_TRUE(f2_.UnprotectRtp(rtp_packet, out_len, &out_len));
    EXPECT_EQ(rtp_len, out_len);
    EXPECT_EQ(0, memcmp(rtp_packet, original_rtp_packet, rtp_len));

    EXPECT_TRUE(f2_.ProtectRtp(rtp_packet, rtp_len,
                               sizeof(rtp_packet), &out_len));
    EXPECT_EQ(out_len, rtp_len + rtp_auth_tag_len(cs2));
    EXPECT_NE(0, memcmp(rtp_packet, original_rtp_packet, rtp_len));
    EXPECT_TRUE(f1_.UnprotectRtp(rtp_packet, out_len, &out_len));
    EXPECT_EQ(rtp_len, out_len);
    EXPECT_EQ(0, memcmp(rtp_packet, original_rtp_packet, rtp_len));

    EXPECT_TRUE(f1_.ProtectRtcp(rtcp_packet, rtcp_len,
                                sizeof(rtcp_packet), &out_len));
    EXPECT_EQ(out_len, rtcp_len + 4 + rtcp_auth_tag_len(cs1));  // NOLINT
    EXPECT_NE(0, memcmp(rtcp_packet, kRtcpReport, rtcp_len));
    EXPECT_TRUE(f2_.UnprotectRtcp(rtcp_packet, out_len, &out_len));
    EXPECT_EQ(rtcp_len, out_len);
    EXPECT_EQ(0, memcmp(rtcp_packet, kRtcpReport, rtcp_len));

    EXPECT_TRUE(f2_.ProtectRtcp(rtcp_packet, rtcp_len,
                                sizeof(rtcp_packet), &out_len));
    EXPECT_EQ(out_len, rtcp_len + 4 + rtcp_auth_tag_len(cs2));  // NOLINT
    EXPECT_NE(0, memcmp(rtcp_packet, kRtcpReport, rtcp_len));
    EXPECT_TRUE(f1_.UnprotectRtcp(rtcp_packet, out_len, &out_len));
    EXPECT_EQ(rtcp_len, out_len);
    EXPECT_EQ(0, memcmp(rtcp_packet, kRtcpReport, rtcp_len));
  }
  cricket::SrtpFilter f1_;
  cricket::SrtpFilter f2_;
  int sequence_number_;
};

// Test that we can set up the session and keys properly.
TEST_F(SrtpFilterTest, TestGoodSetupOneCipherSuite) {
  EXPECT_TRUE(f1_.SetOffer(MakeVector(kTestCryptoParams1), CS_LOCAL));
  EXPECT_TRUE(f1_.SetAnswer(MakeVector(kTestCryptoParams2), CS_REMOTE));
  EXPECT_TRUE(f1_.IsActive());
}

// Test that we can set up things with multiple params.
TEST_F(SrtpFilterTest, TestGoodSetupMultipleCipherSuites) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  offer.push_back(kTestCryptoParams1);
  offer[1].tag = 2;
  offer[1].cipher_suite = CS_AES_CM_128_HMAC_SHA1_32;
  answer[0].tag = 2;
  answer[0].cipher_suite = CS_AES_CM_128_HMAC_SHA1_32;
  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_TRUE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_TRUE(f1_.IsActive());
}

// Test that we handle the cases where crypto is not desired.
TEST_F(SrtpFilterTest, TestGoodSetupNoCipherSuites) {
  std::vector<CryptoParams> offer, answer;
  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_TRUE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_FALSE(f1_.IsActive());
}

// Test that we handle the cases where crypto is not desired by the remote side.
TEST_F(SrtpFilterTest, TestGoodSetupNoAnswerCipherSuites) {
  std::vector<CryptoParams> answer;
  EXPECT_TRUE(f1_.SetOffer(MakeVector(kTestCryptoParams1), CS_LOCAL));
  EXPECT_TRUE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_FALSE(f1_.IsActive());
}

// Test that we fail if we call the functions the wrong way.
TEST_F(SrtpFilterTest, TestBadSetup) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  EXPECT_FALSE(f1_.SetAnswer(answer, CS_LOCAL));
  EXPECT_FALSE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_FALSE(f1_.SetAnswer(answer, CS_LOCAL));
  EXPECT_FALSE(f1_.IsActive());
}

// Test that we can set offer multiple times from the same source.
TEST_F(SrtpFilterTest, TestGoodSetupMultipleOffers) {
  EXPECT_TRUE(f1_.SetOffer(MakeVector(kTestCryptoParams1), CS_LOCAL));
  EXPECT_TRUE(f1_.SetOffer(MakeVector(kTestCryptoParams2), CS_LOCAL));
  EXPECT_TRUE(f1_.SetAnswer(MakeVector(kTestCryptoParams2), CS_REMOTE));
  EXPECT_TRUE(f1_.IsActive());
  EXPECT_TRUE(f1_.SetOffer(MakeVector(kTestCryptoParams1), CS_LOCAL));
  EXPECT_TRUE(f1_.SetOffer(MakeVector(kTestCryptoParams2), CS_LOCAL));
  EXPECT_TRUE(f1_.SetAnswer(MakeVector(kTestCryptoParams2), CS_REMOTE));

  EXPECT_TRUE(f2_.SetOffer(MakeVector(kTestCryptoParams1), CS_REMOTE));
  EXPECT_TRUE(f2_.SetOffer(MakeVector(kTestCryptoParams2), CS_REMOTE));
  EXPECT_TRUE(f2_.SetAnswer(MakeVector(kTestCryptoParams2), CS_LOCAL));
  EXPECT_TRUE(f2_.IsActive());
  EXPECT_TRUE(f2_.SetOffer(MakeVector(kTestCryptoParams1), CS_REMOTE));
  EXPECT_TRUE(f2_.SetOffer(MakeVector(kTestCryptoParams2), CS_REMOTE));
  EXPECT_TRUE(f2_.SetAnswer(MakeVector(kTestCryptoParams2), CS_LOCAL));
}
// Test that we can't set offer multiple times from different sources.
TEST_F(SrtpFilterTest, TestBadSetupMultipleOffers) {
  EXPECT_TRUE(f1_.SetOffer(MakeVector(kTestCryptoParams1), CS_LOCAL));
  EXPECT_FALSE(f1_.SetOffer(MakeVector(kTestCryptoParams2), CS_REMOTE));
  EXPECT_TRUE(f1_.SetAnswer(MakeVector(kTestCryptoParams1), CS_REMOTE));
  EXPECT_TRUE(f1_.IsActive());
  EXPECT_TRUE(f1_.SetOffer(MakeVector(kTestCryptoParams2), CS_LOCAL));
  EXPECT_FALSE(f1_.SetOffer(MakeVector(kTestCryptoParams1), CS_REMOTE));
  EXPECT_TRUE(f1_.SetAnswer(MakeVector(kTestCryptoParams2), CS_REMOTE));

  EXPECT_TRUE(f2_.SetOffer(MakeVector(kTestCryptoParams2), CS_REMOTE));
  EXPECT_FALSE(f2_.SetOffer(MakeVector(kTestCryptoParams1), CS_LOCAL));
  EXPECT_TRUE(f2_.SetAnswer(MakeVector(kTestCryptoParams2), CS_LOCAL));
  EXPECT_TRUE(f2_.IsActive());
  EXPECT_TRUE(f2_.SetOffer(MakeVector(kTestCryptoParams2), CS_REMOTE));
  EXPECT_FALSE(f2_.SetOffer(MakeVector(kTestCryptoParams1), CS_LOCAL));
  EXPECT_TRUE(f2_.SetAnswer(MakeVector(kTestCryptoParams2), CS_LOCAL));
}

// Test that we fail if we have params in the answer when none were offered.
TEST_F(SrtpFilterTest, TestNoAnswerCipherSuites) {
  std::vector<CryptoParams> offer;
  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_FALSE(f1_.SetAnswer(MakeVector(kTestCryptoParams2), CS_REMOTE));
  EXPECT_FALSE(f1_.IsActive());
}

// Test that we fail if we have too many params in our answer.
TEST_F(SrtpFilterTest, TestMultipleAnswerCipherSuites) {
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  answer.push_back(kTestCryptoParams2);
  answer[1].tag = 2;
  answer[1].cipher_suite = CS_AES_CM_128_HMAC_SHA1_32;
  EXPECT_TRUE(f1_.SetOffer(MakeVector(kTestCryptoParams1), CS_LOCAL));
  EXPECT_FALSE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_FALSE(f1_.IsActive());
}

// Test that we fail if we don't support the cipher-suite.
TEST_F(SrtpFilterTest, TestInvalidCipherSuite) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  offer[0].cipher_suite = answer[0].cipher_suite = "FOO";
  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_FALSE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_FALSE(f1_.IsActive());
}

// Test that we fail if we can't agree on a tag.
TEST_F(SrtpFilterTest, TestNoMatchingTag) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  answer[0].tag = 99;
  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_FALSE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_FALSE(f1_.IsActive());
}

// Test that we fail if we can't agree on a cipher-suite.
TEST_F(SrtpFilterTest, TestNoMatchingCipherSuite) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  answer[0].tag = 2;
  answer[0].cipher_suite = "FOO";
  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_FALSE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_FALSE(f1_.IsActive());
}

// Test that we fail keys with bad base64 content.
TEST_F(SrtpFilterTest, TestInvalidKeyData) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  answer[0].key_params = "inline:!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_FALSE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_FALSE(f1_.IsActive());
}

// Test that we fail keys with the wrong key-method.
TEST_F(SrtpFilterTest, TestWrongKeyMethod) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  answer[0].key_params = "outline:PS1uQCVeeCFCanVmcjkpPywjNWhcYD0mXXtxaVBR";
  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_FALSE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_FALSE(f1_.IsActive());
}

// Test that we fail keys of the wrong length.
TEST_F(SrtpFilterTest, TestKeyTooShort) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  answer[0].key_params = "inline:PS1uQCVeeCFCanVmcjkpPywjNWhcYD0mXXtx";
  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_FALSE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_FALSE(f1_.IsActive());
}

// Test that we fail keys of the wrong length.
TEST_F(SrtpFilterTest, TestKeyTooLong) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  answer[0].key_params = "inline:PS1uQCVeeCFCanVmcjkpPywjNWhcYD0mXXtxaVBRABCD";
  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_FALSE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_FALSE(f1_.IsActive());
}

// Test that we fail keys with lifetime or MKI set (since we don't support)
TEST_F(SrtpFilterTest, TestUnsupportedOptions) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  answer[0].key_params =
      "inline:PS1uQCVeeCFCanVmcjkpPywjNWhcYD0mXXtxaVBR|2^20|1:4";
  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_FALSE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_FALSE(f1_.IsActive());
}

// Test that we can encrypt/decrypt after setting the same CryptoParams again on
// one side.
TEST_F(SrtpFilterTest, TestSettingSameKeyOnOneSide) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  TestSetParams(offer, answer);

  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_80,
                       CS_AES_CM_128_HMAC_SHA1_80);

  // Re-applying the same keys on one end and it should not reset the ROC.
  EXPECT_TRUE(f2_.SetOffer(offer, CS_REMOTE));
  EXPECT_TRUE(f2_.SetAnswer(answer, CS_LOCAL));
  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_80, CS_AES_CM_128_HMAC_SHA1_80);
}

// Test that we can encrypt/decrypt after negotiating AES_CM_128_HMAC_SHA1_80.
TEST_F(SrtpFilterTest, TestProtect_AES_CM_128_HMAC_SHA1_80) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  offer.push_back(kTestCryptoParams1);
  offer[1].tag = 2;
  offer[1].cipher_suite = CS_AES_CM_128_HMAC_SHA1_32;
  TestSetParams(offer, answer);
  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_80, CS_AES_CM_128_HMAC_SHA1_80);
}

// Test that we can encrypt/decrypt after negotiating AES_CM_128_HMAC_SHA1_32.
TEST_F(SrtpFilterTest, TestProtect_AES_CM_128_HMAC_SHA1_32) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));
  offer.push_back(kTestCryptoParams1);
  offer[1].tag = 2;
  offer[1].cipher_suite = CS_AES_CM_128_HMAC_SHA1_32;
  answer[0].tag = 2;
  answer[0].cipher_suite = CS_AES_CM_128_HMAC_SHA1_32;
  TestSetParams(offer, answer);
  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_32, CS_AES_CM_128_HMAC_SHA1_32);
}

// Test that we can change encryption parameters.
TEST_F(SrtpFilterTest, TestChangeParameters) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));

  TestSetParams(offer, answer);
  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_80, CS_AES_CM_128_HMAC_SHA1_80);

  // Change the key parameters and cipher_suite.
  offer[0].key_params = kTestKeyParams3;
  offer[0].cipher_suite = CS_AES_CM_128_HMAC_SHA1_32;
  answer[0].key_params = kTestKeyParams4;
  answer[0].cipher_suite = CS_AES_CM_128_HMAC_SHA1_32;

  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_TRUE(f2_.SetOffer(offer, CS_REMOTE));
  EXPECT_TRUE(f1_.IsActive());
  EXPECT_TRUE(f1_.IsActive());

  // Test that the old keys are valid until the negotiation is complete.
  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_80, CS_AES_CM_128_HMAC_SHA1_80);

  // Complete the negotiation and test that we can still understand each other.
  EXPECT_TRUE(f2_.SetAnswer(answer, CS_LOCAL));
  EXPECT_TRUE(f1_.SetAnswer(answer, CS_REMOTE));

  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_32, CS_AES_CM_128_HMAC_SHA1_32);
}

// Test that we can send and receive provisional answers with crypto enabled.
// Also test that we can change the crypto.
TEST_F(SrtpFilterTest, TestProvisionalAnswer) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  offer.push_back(kTestCryptoParams1);
  offer[1].tag = 2;
  offer[1].cipher_suite = CS_AES_CM_128_HMAC_SHA1_32;
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));

  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_TRUE(f2_.SetOffer(offer, CS_REMOTE));
  EXPECT_TRUE(f2_.SetProvisionalAnswer(answer, CS_LOCAL));
  EXPECT_TRUE(f1_.SetProvisionalAnswer(answer, CS_REMOTE));
  EXPECT_TRUE(f1_.IsActive());
  EXPECT_TRUE(f2_.IsActive());
  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_80, CS_AES_CM_128_HMAC_SHA1_80);

  answer[0].key_params = kTestKeyParams4;
  answer[0].tag = 2;
  answer[0].cipher_suite = CS_AES_CM_128_HMAC_SHA1_32;
  EXPECT_TRUE(f2_.SetAnswer(answer, CS_LOCAL));
  EXPECT_TRUE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_TRUE(f1_.IsActive());
  EXPECT_TRUE(f2_.IsActive());
  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_32, CS_AES_CM_128_HMAC_SHA1_32);
}

// Test that a provisional answer doesn't need to contain a crypto.
TEST_F(SrtpFilterTest, TestProvisionalAnswerWithoutCrypto) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer;

  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_TRUE(f2_.SetOffer(offer, CS_REMOTE));
  EXPECT_TRUE(f2_.SetProvisionalAnswer(answer, CS_LOCAL));
  EXPECT_TRUE(f1_.SetProvisionalAnswer(answer, CS_REMOTE));
  EXPECT_FALSE(f1_.IsActive());
  EXPECT_FALSE(f2_.IsActive());

  answer.push_back(kTestCryptoParams2);
  EXPECT_TRUE(f2_.SetAnswer(answer, CS_LOCAL));
  EXPECT_TRUE(f1_.SetAnswer(answer, CS_REMOTE));
  EXPECT_TRUE(f1_.IsActive());
  EXPECT_TRUE(f2_.IsActive());
  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_80, CS_AES_CM_128_HMAC_SHA1_80);
}

// Test that we can disable encryption.
TEST_F(SrtpFilterTest, TestDisableEncryption) {
  std::vector<CryptoParams> offer(MakeVector(kTestCryptoParams1));
  std::vector<CryptoParams> answer(MakeVector(kTestCryptoParams2));

  TestSetParams(offer, answer);
  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_80, CS_AES_CM_128_HMAC_SHA1_80);

  offer.clear();
  answer.clear();
  EXPECT_TRUE(f1_.SetOffer(offer, CS_LOCAL));
  EXPECT_TRUE(f2_.SetOffer(offer, CS_REMOTE));
  EXPECT_TRUE(f1_.IsActive());
  EXPECT_TRUE(f2_.IsActive());

  // Test that the old keys are valid until the negotiation is complete.
  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_80, CS_AES_CM_128_HMAC_SHA1_80);

  // Complete the negotiation.
  EXPECT_TRUE(f2_.SetAnswer(answer, CS_LOCAL));
  EXPECT_TRUE(f1_.SetAnswer(answer, CS_REMOTE));

  EXPECT_FALSE(f1_.IsActive());
  EXPECT_FALSE(f2_.IsActive());
}

// Test directly setting the params with AES_CM_128_HMAC_SHA1_80
TEST_F(SrtpFilterTest, TestProtect_SetParamsDirect_AES_CM_128_HMAC_SHA1_80) {
  EXPECT_TRUE(f1_.SetRtpParams(CS_AES_CM_128_HMAC_SHA1_80,
                               kTestKey1, kTestKeyLen,
                               CS_AES_CM_128_HMAC_SHA1_80,
                               kTestKey2, kTestKeyLen));
  EXPECT_TRUE(f2_.SetRtpParams(CS_AES_CM_128_HMAC_SHA1_80,
                               kTestKey2, kTestKeyLen,
                               CS_AES_CM_128_HMAC_SHA1_80,
                               kTestKey1, kTestKeyLen));
  EXPECT_TRUE(f1_.SetRtcpParams(CS_AES_CM_128_HMAC_SHA1_80,
                                kTestKey1, kTestKeyLen,
                                CS_AES_CM_128_HMAC_SHA1_80,
                                kTestKey2, kTestKeyLen));
  EXPECT_TRUE(f2_.SetRtcpParams(CS_AES_CM_128_HMAC_SHA1_80,
                                kTestKey2, kTestKeyLen,
                                CS_AES_CM_128_HMAC_SHA1_80,
                                kTestKey1, kTestKeyLen));
  EXPECT_TRUE(f1_.IsActive());
  EXPECT_TRUE(f2_.IsActive());
  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_80, CS_AES_CM_128_HMAC_SHA1_80);
}

// Test directly setting the params with AES_CM_128_HMAC_SHA1_32
TEST_F(SrtpFilterTest, TestProtect_SetParamsDirect_AES_CM_128_HMAC_SHA1_32) {
  EXPECT_TRUE(f1_.SetRtpParams(CS_AES_CM_128_HMAC_SHA1_32,
                               kTestKey1, kTestKeyLen,
                               CS_AES_CM_128_HMAC_SHA1_32,
                               kTestKey2, kTestKeyLen));
  EXPECT_TRUE(f2_.SetRtpParams(CS_AES_CM_128_HMAC_SHA1_32,
                               kTestKey2, kTestKeyLen,
                               CS_AES_CM_128_HMAC_SHA1_32,
                               kTestKey1, kTestKeyLen));
  EXPECT_TRUE(f1_.SetRtcpParams(CS_AES_CM_128_HMAC_SHA1_32,
                                kTestKey1, kTestKeyLen,
                                CS_AES_CM_128_HMAC_SHA1_32,
                                kTestKey2, kTestKeyLen));
  EXPECT_TRUE(f2_.SetRtcpParams(CS_AES_CM_128_HMAC_SHA1_32,
                                kTestKey2, kTestKeyLen,
                                CS_AES_CM_128_HMAC_SHA1_32,
                                kTestKey1, kTestKeyLen));
  EXPECT_TRUE(f1_.IsActive());
  EXPECT_TRUE(f2_.IsActive());
  TestProtectUnprotect(CS_AES_CM_128_HMAC_SHA1_32, CS_AES_CM_128_HMAC_SHA1_32);
}

// Test directly setting the params with bogus keys
TEST_F(SrtpFilterTest, TestSetParamsKeyTooShort) {
  EXPECT_FALSE(f1_.SetRtpParams(CS_AES_CM_128_HMAC_SHA1_80,
                                kTestKey1, kTestKeyLen - 1,
                                CS_AES_CM_128_HMAC_SHA1_80,
                                kTestKey1, kTestKeyLen - 1));
  EXPECT_FALSE(f1_.SetRtcpParams(CS_AES_CM_128_HMAC_SHA1_80,
                                 kTestKey1, kTestKeyLen - 1,
                                 CS_AES_CM_128_HMAC_SHA1_80,
                                 kTestKey1, kTestKeyLen - 1));
}

#if defined(ENABLE_EXTERNAL_AUTH)
TEST_F(SrtpFilterTest, TestGetSendAuthParams) {
  EXPECT_TRUE(f1_.SetRtpParams(CS_AES_CM_128_HMAC_SHA1_32,
                               kTestKey1, kTestKeyLen,
                               CS_AES_CM_128_HMAC_SHA1_32,
                               kTestKey2, kTestKeyLen));
  EXPECT_TRUE(f1_.SetRtcpParams(CS_AES_CM_128_HMAC_SHA1_32,
                                kTestKey1, kTestKeyLen,
                                CS_AES_CM_128_HMAC_SHA1_32,
                                kTestKey2, kTestKeyLen));
  uint8* auth_key = NULL;
  int auth_key_len = 0, auth_tag_len = 0;
  EXPECT_TRUE(f1_.GetRtpAuthParams(&auth_key, &auth_key_len, &auth_tag_len));
  EXPECT_TRUE(auth_key != NULL);
  EXPECT_EQ(20, auth_key_len);
  EXPECT_EQ(4, auth_tag_len);
}
#endif

class SrtpSessionTest : public testing::Test {
 protected:
  virtual void SetUp() {
    rtp_len_ = sizeof(kPcmuFrame);
    rtcp_len_ = sizeof(kRtcpReport);
    memcpy(rtp_packet_, kPcmuFrame, rtp_len_);
    memcpy(rtcp_packet_, kRtcpReport, rtcp_len_);
  }
  void TestProtectRtp(const std::string& cs) {
    int out_len = 0;
    EXPECT_TRUE(s1_.ProtectRtp(rtp_packet_, rtp_len_,
                               sizeof(rtp_packet_), &out_len));
    EXPECT_EQ(out_len, rtp_len_ + rtp_auth_tag_len(cs));
    EXPECT_NE(0, memcmp(rtp_packet_, kPcmuFrame, rtp_len_));
    rtp_len_ = out_len;
  }
  void TestProtectRtcp(const std::string& cs) {
    int out_len = 0;
    EXPECT_TRUE(s1_.ProtectRtcp(rtcp_packet_, rtcp_len_,
                                sizeof(rtcp_packet_), &out_len));
    EXPECT_EQ(out_len, rtcp_len_ + 4 + rtcp_auth_tag_len(cs));  // NOLINT
    EXPECT_NE(0, memcmp(rtcp_packet_, kRtcpReport, rtcp_len_));
    rtcp_len_ = out_len;
  }
  void TestUnprotectRtp(const std::string& cs) {
    int out_len = 0, expected_len = sizeof(kPcmuFrame);
    EXPECT_TRUE(s2_.UnprotectRtp(rtp_packet_, rtp_len_, &out_len));
    EXPECT_EQ(expected_len, out_len);
    EXPECT_EQ(0, memcmp(rtp_packet_, kPcmuFrame, out_len));
  }
  void TestUnprotectRtcp(const std::string& cs) {
    int out_len = 0, expected_len = sizeof(kRtcpReport);
    EXPECT_TRUE(s2_.UnprotectRtcp(rtcp_packet_, rtcp_len_, &out_len));
    EXPECT_EQ(expected_len, out_len);
    EXPECT_EQ(0, memcmp(rtcp_packet_, kRtcpReport, out_len));
  }
  cricket::SrtpSession s1_;
  cricket::SrtpSession s2_;
  char rtp_packet_[sizeof(kPcmuFrame) + 10];
  char rtcp_packet_[sizeof(kRtcpReport) + 4 + 10];
  int rtp_len_;
  int rtcp_len_;
};

// Test that we can set up the session and keys properly.
TEST_F(SrtpSessionTest, TestGoodSetup) {
  EXPECT_TRUE(s1_.SetSend(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, kTestKeyLen));
  EXPECT_TRUE(s2_.SetRecv(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, kTestKeyLen));
}

// Test that we can't change the keys once set.
TEST_F(SrtpSessionTest, TestBadSetup) {
  EXPECT_TRUE(s1_.SetSend(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, kTestKeyLen));
  EXPECT_TRUE(s2_.SetRecv(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, kTestKeyLen));
  EXPECT_FALSE(s1_.SetSend(CS_AES_CM_128_HMAC_SHA1_80, kTestKey2, kTestKeyLen));
  EXPECT_FALSE(s2_.SetRecv(CS_AES_CM_128_HMAC_SHA1_80, kTestKey2, kTestKeyLen));
}

// Test that we fail keys of the wrong length.
TEST_F(SrtpSessionTest, TestKeysTooShort) {
  EXPECT_FALSE(s1_.SetSend(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, 1));
  EXPECT_FALSE(s2_.SetRecv(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, 1));
}

// Test that we can encrypt and decrypt RTP/RTCP using AES_CM_128_HMAC_SHA1_80.
TEST_F(SrtpSessionTest, TestProtect_AES_CM_128_HMAC_SHA1_80) {
  EXPECT_TRUE(s1_.SetSend(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, kTestKeyLen));
  EXPECT_TRUE(s2_.SetRecv(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, kTestKeyLen));
  TestProtectRtp(CS_AES_CM_128_HMAC_SHA1_80);
  TestProtectRtcp(CS_AES_CM_128_HMAC_SHA1_80);
  TestUnprotectRtp(CS_AES_CM_128_HMAC_SHA1_80);
  TestUnprotectRtcp(CS_AES_CM_128_HMAC_SHA1_80);
}

// Test that we can encrypt and decrypt RTP/RTCP using AES_CM_128_HMAC_SHA1_32.
TEST_F(SrtpSessionTest, TestProtect_AES_CM_128_HMAC_SHA1_32) {
  EXPECT_TRUE(s1_.SetSend(CS_AES_CM_128_HMAC_SHA1_32, kTestKey1, kTestKeyLen));
  EXPECT_TRUE(s2_.SetRecv(CS_AES_CM_128_HMAC_SHA1_32, kTestKey1, kTestKeyLen));
  TestProtectRtp(CS_AES_CM_128_HMAC_SHA1_32);
  TestProtectRtcp(CS_AES_CM_128_HMAC_SHA1_32);
  TestUnprotectRtp(CS_AES_CM_128_HMAC_SHA1_32);
  TestUnprotectRtcp(CS_AES_CM_128_HMAC_SHA1_32);
}

TEST_F(SrtpSessionTest, TestGetSendStreamPacketIndex) {
  EXPECT_TRUE(s1_.SetSend(CS_AES_CM_128_HMAC_SHA1_32, kTestKey1, kTestKeyLen));
  int64 index;
  int out_len = 0;
  EXPECT_TRUE(s1_.ProtectRtp(rtp_packet_, rtp_len_,
                             sizeof(rtp_packet_), &out_len, &index));
  // |index| will be shifted by 16.
  int64 be64_index = be64_to_cpu(1 << 16);
  EXPECT_EQ(be64_index, index);
}

// Test that we fail to unprotect if someone tampers with the RTP/RTCP paylaods.
TEST_F(SrtpSessionTest, TestTamperReject) {
  int out_len;
  EXPECT_TRUE(s1_.SetSend(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, kTestKeyLen));
  EXPECT_TRUE(s2_.SetRecv(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, kTestKeyLen));
  TestProtectRtp(CS_AES_CM_128_HMAC_SHA1_80);
  TestProtectRtcp(CS_AES_CM_128_HMAC_SHA1_80);
  rtp_packet_[0] = 0x12;
  rtcp_packet_[1] = 0x34;
  EXPECT_FALSE(s2_.UnprotectRtp(rtp_packet_, rtp_len_, &out_len));
  EXPECT_FALSE(s2_.UnprotectRtcp(rtcp_packet_, rtcp_len_, &out_len));
}

// Test that we fail to unprotect if the payloads are not authenticated.
TEST_F(SrtpSessionTest, TestUnencryptReject) {
  int out_len;
  EXPECT_TRUE(s1_.SetSend(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, kTestKeyLen));
  EXPECT_TRUE(s2_.SetRecv(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, kTestKeyLen));
  EXPECT_FALSE(s2_.UnprotectRtp(rtp_packet_, rtp_len_, &out_len));
  EXPECT_FALSE(s2_.UnprotectRtcp(rtcp_packet_, rtcp_len_, &out_len));
}

// Test that we fail when using buffers that are too small.
TEST_F(SrtpSessionTest, TestBuffersTooSmall) {
  int out_len;
  EXPECT_TRUE(s1_.SetSend(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, kTestKeyLen));
  EXPECT_FALSE(s1_.ProtectRtp(rtp_packet_, rtp_len_,
                              sizeof(rtp_packet_) - 10, &out_len));
  EXPECT_FALSE(s1_.ProtectRtcp(rtcp_packet_, rtcp_len_,
                               sizeof(rtcp_packet_) - 14, &out_len));
}

TEST_F(SrtpSessionTest, TestReplay) {
  static const uint16 kMaxSeqnum = static_cast<uint16>(-1);
  static const uint16 seqnum_big = 62275;
  static const uint16 seqnum_small = 10;
  static const uint16 replay_window = 1024;
  int out_len;

  EXPECT_TRUE(s1_.SetSend(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, kTestKeyLen));
  EXPECT_TRUE(s2_.SetRecv(CS_AES_CM_128_HMAC_SHA1_80, kTestKey1, kTestKeyLen));

  // Initial sequence number.
  talk_base::SetBE16(reinterpret_cast<uint8*>(rtp_packet_) + 2, seqnum_big);
  EXPECT_TRUE(s1_.ProtectRtp(rtp_packet_, rtp_len_, sizeof(rtp_packet_),
                             &out_len));

  // Replay within the 1024 window should succeed.
  talk_base::SetBE16(reinterpret_cast<uint8*>(rtp_packet_) + 2,
                     seqnum_big - replay_window + 1);
  EXPECT_TRUE(s1_.ProtectRtp(rtp_packet_, rtp_len_, sizeof(rtp_packet_),
                             &out_len));

  // Replay out side of the 1024 window should fail.
  talk_base::SetBE16(reinterpret_cast<uint8*>(rtp_packet_) + 2,
                     seqnum_big - replay_window - 1);
  EXPECT_FALSE(s1_.ProtectRtp(rtp_packet_, rtp_len_, sizeof(rtp_packet_),
                              &out_len));

  // Increment sequence number to a small number.
  talk_base::SetBE16(reinterpret_cast<uint8*>(rtp_packet_) + 2, seqnum_small);
  EXPECT_TRUE(s1_.ProtectRtp(rtp_packet_, rtp_len_, sizeof(rtp_packet_),
                             &out_len));

  // Replay around 0 but out side of the 1024 window should fail.
  talk_base::SetBE16(reinterpret_cast<uint8*>(rtp_packet_) + 2,
                     kMaxSeqnum + seqnum_small - replay_window - 1);
  EXPECT_FALSE(s1_.ProtectRtp(rtp_packet_, rtp_len_, sizeof(rtp_packet_),
                              &out_len));

  // Replay around 0 but within the 1024 window should succeed.
  for (uint16 seqnum = 65000; seqnum < 65003; ++seqnum) {
    talk_base::SetBE16(reinterpret_cast<uint8*>(rtp_packet_) + 2, seqnum);
    EXPECT_TRUE(s1_.ProtectRtp(rtp_packet_, rtp_len_, sizeof(rtp_packet_),
                               &out_len));
  }

  // Go back to normal sequence nubmer.
  // NOTE: without the fix in libsrtp, this would fail. This is because
  // without the fix, the loop above would keep incrementing local sequence
  // number in libsrtp, eventually the new sequence number would go out side
  // of the window.
  talk_base::SetBE16(reinterpret_cast<uint8*>(rtp_packet_) + 2,
                     seqnum_small + 1);
  EXPECT_TRUE(s1_.ProtectRtp(rtp_packet_, rtp_len_, sizeof(rtp_packet_),
                             &out_len));
}

class SrtpStatTest
    : public testing::Test,
      public sigslot::has_slots<> {
 public:
  SrtpStatTest()
      : ssrc_(0U),
        mode_(-1),
        error_(cricket::SrtpFilter::ERROR_NONE) {
    srtp_stat_.SignalSrtpError.connect(this, &SrtpStatTest::OnSrtpError);
    srtp_stat_.set_signal_silent_time(200);
  }

 protected:
  void OnSrtpError(uint32 ssrc, cricket::SrtpFilter::Mode mode,
                   cricket::SrtpFilter::Error error) {
    ssrc_ = ssrc;
    mode_ = mode;
    error_ = error;
  }
  void Reset() {
    ssrc_ = 0U;
    mode_ = -1;
    error_ = cricket::SrtpFilter::ERROR_NONE;
  }

  cricket::SrtpStat srtp_stat_;
  uint32 ssrc_;
  int mode_;
  cricket::SrtpFilter::Error error_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SrtpStatTest);
};

TEST_F(SrtpStatTest, TestProtectRtpError) {
  Reset();
  srtp_stat_.AddProtectRtpResult(1, err_status_ok);
  EXPECT_EQ(0U, ssrc_);
  EXPECT_EQ(-1, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_NONE, error_);
  Reset();
  srtp_stat_.AddProtectRtpResult(1, err_status_auth_fail);
  EXPECT_EQ(1U, ssrc_);
  EXPECT_EQ(cricket::SrtpFilter::PROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_AUTH, error_);
  Reset();
  srtp_stat_.AddProtectRtpResult(1, err_status_fail);
  EXPECT_EQ(1U, ssrc_);
  EXPECT_EQ(cricket::SrtpFilter::PROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_FAIL, error_);
  // Within 200ms, the error will not be triggered.
  Reset();
  srtp_stat_.AddProtectRtpResult(1, err_status_fail);
  EXPECT_EQ(0U, ssrc_);
  EXPECT_EQ(-1, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_NONE, error_);
  // Now the error will be triggered again.
  Reset();
  talk_base::Thread::Current()->SleepMs(210);
  srtp_stat_.AddProtectRtpResult(1, err_status_fail);
  EXPECT_EQ(1U, ssrc_);
  EXPECT_EQ(cricket::SrtpFilter::PROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_FAIL, error_);
}

TEST_F(SrtpStatTest, TestUnprotectRtpError) {
  Reset();
  srtp_stat_.AddUnprotectRtpResult(1, err_status_ok);
  EXPECT_EQ(0U, ssrc_);
  EXPECT_EQ(-1, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_NONE, error_);
  Reset();
  srtp_stat_.AddUnprotectRtpResult(1, err_status_auth_fail);
  EXPECT_EQ(1U, ssrc_);
  EXPECT_EQ(cricket::SrtpFilter::UNPROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_AUTH, error_);
  Reset();
  srtp_stat_.AddUnprotectRtpResult(1, err_status_replay_fail);
  EXPECT_EQ(1U, ssrc_);
  EXPECT_EQ(cricket::SrtpFilter::UNPROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_REPLAY, error_);
  Reset();
  talk_base::Thread::Current()->SleepMs(210);
  srtp_stat_.AddUnprotectRtpResult(1, err_status_replay_old);
  EXPECT_EQ(1U, ssrc_);
  EXPECT_EQ(cricket::SrtpFilter::UNPROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_REPLAY, error_);
  Reset();
  srtp_stat_.AddUnprotectRtpResult(1, err_status_fail);
  EXPECT_EQ(1U, ssrc_);
  EXPECT_EQ(cricket::SrtpFilter::UNPROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_FAIL, error_);
  // Within 200ms, the error will not be triggered.
  Reset();
  srtp_stat_.AddUnprotectRtpResult(1, err_status_fail);
  EXPECT_EQ(0U, ssrc_);
  EXPECT_EQ(-1, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_NONE, error_);
  // Now the error will be triggered again.
  Reset();
  talk_base::Thread::Current()->SleepMs(210);
  srtp_stat_.AddUnprotectRtpResult(1, err_status_fail);
  EXPECT_EQ(1U, ssrc_);
  EXPECT_EQ(cricket::SrtpFilter::UNPROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_FAIL, error_);
}

TEST_F(SrtpStatTest, TestProtectRtcpError) {
  Reset();
  srtp_stat_.AddProtectRtcpResult(err_status_ok);
  EXPECT_EQ(-1, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_NONE, error_);
  Reset();
  srtp_stat_.AddProtectRtcpResult(err_status_auth_fail);
  EXPECT_EQ(cricket::SrtpFilter::PROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_AUTH, error_);
  Reset();
  srtp_stat_.AddProtectRtcpResult(err_status_fail);
  EXPECT_EQ(cricket::SrtpFilter::PROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_FAIL, error_);
  // Within 200ms, the error will not be triggered.
  Reset();
  srtp_stat_.AddProtectRtcpResult(err_status_fail);
  EXPECT_EQ(-1, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_NONE, error_);
  // Now the error will be triggered again.
  Reset();
  talk_base::Thread::Current()->SleepMs(210);
  srtp_stat_.AddProtectRtcpResult(err_status_fail);
  EXPECT_EQ(cricket::SrtpFilter::PROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_FAIL, error_);
}

TEST_F(SrtpStatTest, TestUnprotectRtcpError) {
  Reset();
  srtp_stat_.AddUnprotectRtcpResult(err_status_ok);
  EXPECT_EQ(-1, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_NONE, error_);
  Reset();
  srtp_stat_.AddUnprotectRtcpResult(err_status_auth_fail);
  EXPECT_EQ(cricket::SrtpFilter::UNPROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_AUTH, error_);
  Reset();
  srtp_stat_.AddUnprotectRtcpResult(err_status_replay_fail);
  EXPECT_EQ(cricket::SrtpFilter::UNPROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_REPLAY, error_);
  Reset();
  talk_base::Thread::Current()->SleepMs(210);
  srtp_stat_.AddUnprotectRtcpResult(err_status_replay_fail);
  EXPECT_EQ(cricket::SrtpFilter::UNPROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_REPLAY, error_);
  Reset();
  srtp_stat_.AddUnprotectRtcpResult(err_status_fail);
  EXPECT_EQ(cricket::SrtpFilter::UNPROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_FAIL, error_);
  // Within 200ms, the error will not be triggered.
  Reset();
  srtp_stat_.AddUnprotectRtcpResult(err_status_fail);
  EXPECT_EQ(-1, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_NONE, error_);
  // Now the error will be triggered again.
  Reset();
  talk_base::Thread::Current()->SleepMs(210);
  srtp_stat_.AddUnprotectRtcpResult(err_status_fail);
  EXPECT_EQ(cricket::SrtpFilter::UNPROTECT, mode_);
  EXPECT_EQ(cricket::SrtpFilter::ERROR_FAIL, error_);
}
