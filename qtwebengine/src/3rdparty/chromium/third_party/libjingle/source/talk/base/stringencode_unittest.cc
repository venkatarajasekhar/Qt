/*
 * libjingle
 * Copyright 2004, Google Inc.
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

#include "talk/base/common.h"
#include "talk/base/gunit.h"
#include "talk/base/stringencode.h"
#include "talk/base/stringutils.h"

namespace talk_base {

TEST(Utf8EncodeTest, EncodeDecode) {
  const struct Utf8Test {
    const char* encoded;
    size_t encsize, enclen;
    unsigned long decoded;
  } kTests[] = {
    { "a    ",             5, 1, 'a' },
    { "\x7F    ",          5, 1, 0x7F },
    { "\xC2\x80   ",       5, 2, 0x80 },
    { "\xDF\xBF   ",       5, 2, 0x7FF },
    { "\xE0\xA0\x80  ",    5, 3, 0x800 },
    { "\xEF\xBF\xBF  ",    5, 3, 0xFFFF },
    { "\xF0\x90\x80\x80 ", 5, 4, 0x10000 },
    { "\xF0\x90\x80\x80 ", 3, 0, 0x10000 },
    { "\xF0\xF0\x80\x80 ", 5, 0, 0 },
    { "\xF0\x90\x80  ",    5, 0, 0 },
    { "\x90\x80\x80  ",    5, 0, 0 },
    { NULL, 0, 0 },
  };
  for (size_t i = 0; kTests[i].encoded; ++i) {
    unsigned long val = 0;
    ASSERT_EQ(kTests[i].enclen, utf8_decode(kTests[i].encoded,
                                            kTests[i].encsize,
                                            &val));
    unsigned long result = (kTests[i].enclen == 0) ? 0 : kTests[i].decoded;
    ASSERT_EQ(result, val);

    if (kTests[i].decoded == 0) {
      // Not an interesting encoding test case
      continue;
    }

    char buffer[5];
    memset(buffer, 0x01, ARRAY_SIZE(buffer));
    ASSERT_EQ(kTests[i].enclen, utf8_encode(buffer,
                                            kTests[i].encsize,
                                            kTests[i].decoded));
    ASSERT_TRUE(memcmp(buffer, kTests[i].encoded, kTests[i].enclen) == 0);
    // Make sure remainder of buffer is unchanged
    ASSERT_TRUE(memory_check(buffer + kTests[i].enclen,
                             0x1,
                             ARRAY_SIZE(buffer) - kTests[i].enclen));
  }
}

class HexEncodeTest : public testing::Test {
 public:
  HexEncodeTest() : enc_res_(0), dec_res_(0) {
    for (size_t i = 0; i < sizeof(data_); ++i) {
      data_[i] = (i + 128) & 0xff;
    }
    memset(decoded_, 0x7f, sizeof(decoded_));
  }

  char data_[10];
  char encoded_[31];
  char decoded_[11];
  size_t enc_res_;
  size_t dec_res_;
};

// Test that we can convert to/from hex with no delimiter.
TEST_F(HexEncodeTest, TestWithNoDelimiter) {
  enc_res_ = hex_encode(encoded_, sizeof(encoded_), data_, sizeof(data_));
  ASSERT_EQ(sizeof(data_) * 2, enc_res_);
  ASSERT_STREQ("80818283848586878889", encoded_);
  dec_res_ = hex_decode(decoded_, sizeof(decoded_), encoded_, enc_res_);
  ASSERT_EQ(sizeof(data_), dec_res_);
  ASSERT_EQ(0, memcmp(data_, decoded_, dec_res_));
}

// Test that we can convert to/from hex with a colon delimiter.
TEST_F(HexEncodeTest, TestWithDelimiter) {
  enc_res_ = hex_encode_with_delimiter(encoded_, sizeof(encoded_),
                                       data_, sizeof(data_), ':');
  ASSERT_EQ(sizeof(data_) * 3 - 1, enc_res_);
  ASSERT_STREQ("80:81:82:83:84:85:86:87:88:89", encoded_);
  dec_res_ = hex_decode_with_delimiter(decoded_, sizeof(decoded_),
                                       encoded_, enc_res_, ':');
  ASSERT_EQ(sizeof(data_), dec_res_);
  ASSERT_EQ(0, memcmp(data_, decoded_, dec_res_));
}

// Test that encoding with one delimiter and decoding with another fails.
TEST_F(HexEncodeTest, TestWithWrongDelimiter) {
  enc_res_ = hex_encode_with_delimiter(encoded_, sizeof(encoded_),
                                       data_, sizeof(data_), ':');
  ASSERT_EQ(sizeof(data_) * 3 - 1, enc_res_);
  dec_res_ = hex_decode_with_delimiter(decoded_, sizeof(decoded_),
                                       encoded_, enc_res_, '/');
  ASSERT_EQ(0U, dec_res_);
}

// Test that encoding without a delimiter and decoding with one fails.
TEST_F(HexEncodeTest, TestExpectedDelimiter) {
  enc_res_ = hex_encode(encoded_, sizeof(encoded_), data_, sizeof(data_));
  ASSERT_EQ(sizeof(data_) * 2, enc_res_);
  dec_res_ = hex_decode_with_delimiter(decoded_, sizeof(decoded_),
                                       encoded_, enc_res_, ':');
  ASSERT_EQ(0U, dec_res_);
}

// Test that encoding with a delimiter and decoding without one fails.
TEST_F(HexEncodeTest, TestExpectedNoDelimiter) {
  enc_res_ = hex_encode_with_delimiter(encoded_, sizeof(encoded_),
                                       data_, sizeof(data_), ':');
  ASSERT_EQ(sizeof(data_) * 3 - 1, enc_res_);
  dec_res_ = hex_decode(decoded_, sizeof(decoded_), encoded_, enc_res_);
  ASSERT_EQ(0U, dec_res_);
}

// Test that we handle a zero-length buffer with no delimiter.
TEST_F(HexEncodeTest, TestZeroLengthNoDelimiter) {
  enc_res_ = hex_encode(encoded_, sizeof(encoded_), "", 0);
  ASSERT_EQ(0U, enc_res_);
  dec_res_ = hex_decode(decoded_, sizeof(decoded_), encoded_, enc_res_);
  ASSERT_EQ(0U, dec_res_);
}

// Test that we handle a zero-length buffer with a delimiter.
TEST_F(HexEncodeTest, TestZeroLengthWithDelimiter) {
  enc_res_ = hex_encode_with_delimiter(encoded_, sizeof(encoded_), "", 0, ':');
  ASSERT_EQ(0U, enc_res_);
  dec_res_ = hex_decode_with_delimiter(decoded_, sizeof(decoded_),
                                       encoded_, enc_res_, ':');
  ASSERT_EQ(0U, dec_res_);
}

// Test the std::string variants that take no delimiter.
TEST_F(HexEncodeTest, TestHelpersNoDelimiter) {
  std::string result = hex_encode(data_, sizeof(data_));
  ASSERT_EQ("80818283848586878889", result);
  dec_res_ = hex_decode(decoded_, sizeof(decoded_), result);
  ASSERT_EQ(sizeof(data_), dec_res_);
  ASSERT_EQ(0, memcmp(data_, decoded_, dec_res_));
}

// Test the std::string variants that use a delimiter.
TEST_F(HexEncodeTest, TestHelpersWithDelimiter) {
  std::string result = hex_encode_with_delimiter(data_, sizeof(data_), ':');
  ASSERT_EQ("80:81:82:83:84:85:86:87:88:89", result);
  dec_res_ = hex_decode_with_delimiter(decoded_, sizeof(decoded_), result, ':');
  ASSERT_EQ(sizeof(data_), dec_res_);
  ASSERT_EQ(0, memcmp(data_, decoded_, dec_res_));
}

// Test that encoding into a too-small output buffer (without delimiter) fails.
TEST_F(HexEncodeTest, TestEncodeTooShort) {
  enc_res_ = hex_encode_with_delimiter(encoded_, sizeof(data_) * 2,
                                       data_, sizeof(data_), 0);
  ASSERT_EQ(0U, enc_res_);
}

// Test that encoding into a too-small output buffer (with delimiter) fails.
TEST_F(HexEncodeTest, TestEncodeWithDelimiterTooShort) {
  enc_res_ = hex_encode_with_delimiter(encoded_, sizeof(data_) * 3 - 1,
                                       data_, sizeof(data_), ':');
  ASSERT_EQ(0U, enc_res_);
}

// Test that decoding into a too-small output buffer fails.
TEST_F(HexEncodeTest, TestDecodeTooShort) {
  dec_res_ = hex_decode_with_delimiter(decoded_, 4, "0123456789", 10, 0);
  ASSERT_EQ(0U, dec_res_);
  ASSERT_EQ(0x7f, decoded_[4]);
}

// Test that decoding non-hex data fails.
TEST_F(HexEncodeTest, TestDecodeBogusData) {
  dec_res_ = hex_decode_with_delimiter(decoded_, sizeof(decoded_), "xyz", 3, 0);
  ASSERT_EQ(0U, dec_res_);
}

// Test that decoding an odd number of hex characters fails.
TEST_F(HexEncodeTest, TestDecodeOddHexDigits) {
  dec_res_ = hex_decode_with_delimiter(decoded_, sizeof(decoded_), "012", 3, 0);
  ASSERT_EQ(0U, dec_res_);
}

// Test that decoding a string with too many delimiters fails.
TEST_F(HexEncodeTest, TestDecodeWithDelimiterTooManyDelimiters) {
  dec_res_ = hex_decode_with_delimiter(decoded_, 4, "01::23::45::67", 14, ':');
  ASSERT_EQ(0U, dec_res_);
}

// Test that decoding a string with a leading delimiter fails.
TEST_F(HexEncodeTest, TestDecodeWithDelimiterLeadingDelimiter) {
  dec_res_ = hex_decode_with_delimiter(decoded_, 4, ":01:23:45:67", 12, ':');
  ASSERT_EQ(0U, dec_res_);
}

// Test that decoding a string with a trailing delimiter fails.
TEST_F(HexEncodeTest, TestDecodeWithDelimiterTrailingDelimiter) {
  dec_res_ = hex_decode_with_delimiter(decoded_, 4, "01:23:45:67:", 12, ':');
  ASSERT_EQ(0U, dec_res_);
}

// Tests counting substrings.
TEST(TokenizeTest, CountSubstrings) {
  std::vector<std::string> fields;

  EXPECT_EQ(5ul, tokenize("one two three four five", ' ', &fields));
  fields.clear();
  EXPECT_EQ(1ul, tokenize("one", ' ', &fields));

  // Extra spaces should be ignored.
  fields.clear();
  EXPECT_EQ(5ul, tokenize("  one    two  three    four five  ", ' ', &fields));
  fields.clear();
  EXPECT_EQ(1ul, tokenize("  one  ", ' ', &fields));
  fields.clear();
  EXPECT_EQ(0ul, tokenize(" ", ' ', &fields));
}

// Tests comparing substrings.
TEST(TokenizeTest, CompareSubstrings) {
  std::vector<std::string> fields;

  tokenize("find middle one", ' ', &fields);
  ASSERT_EQ(3ul, fields.size());
  ASSERT_STREQ("middle", fields.at(1).c_str());
  fields.clear();

  // Extra spaces should be ignored.
  tokenize("  find   middle  one    ", ' ', &fields);
  ASSERT_EQ(3ul, fields.size());
  ASSERT_STREQ("middle", fields.at(1).c_str());
  fields.clear();
  tokenize(" ", ' ', &fields);
  ASSERT_EQ(0ul, fields.size());
}

TEST(TokenizeTest, TokenizeAppend) {
  ASSERT_EQ(0ul, tokenize_append("A B C", ' ', NULL));

  std::vector<std::string> fields;

  tokenize_append("A B C", ' ', &fields);
  ASSERT_EQ(3ul, fields.size());
  ASSERT_STREQ("B", fields.at(1).c_str());

  tokenize_append("D E", ' ', &fields);
  ASSERT_EQ(5ul, fields.size());
  ASSERT_STREQ("B", fields.at(1).c_str());
  ASSERT_STREQ("E", fields.at(4).c_str());
}

TEST(TokenizeTest, TokenizeWithMarks) {
  ASSERT_EQ(0ul, tokenize("D \"A B", ' ', '(', ')', NULL));

  std::vector<std::string> fields;
  tokenize("A B C", ' ', '"', '"', &fields);
  ASSERT_EQ(3ul, fields.size());
  ASSERT_STREQ("C", fields.at(2).c_str());

  tokenize("\"A B\" C", ' ', '"', '"', &fields);
  ASSERT_EQ(2ul, fields.size());
  ASSERT_STREQ("A B", fields.at(0).c_str());

  tokenize("D \"A B\" C", ' ', '"', '"', &fields);
  ASSERT_EQ(3ul, fields.size());
  ASSERT_STREQ("D", fields.at(0).c_str());
  ASSERT_STREQ("A B", fields.at(1).c_str());

  tokenize("D \"A B\" C \"E F\"", ' ', '"', '"', &fields);
  ASSERT_EQ(4ul, fields.size());
  ASSERT_STREQ("D", fields.at(0).c_str());
  ASSERT_STREQ("A B", fields.at(1).c_str());
  ASSERT_STREQ("E F", fields.at(3).c_str());

  // No matching marks.
  tokenize("D \"A B", ' ', '"', '"', &fields);
  ASSERT_EQ(3ul, fields.size());
  ASSERT_STREQ("D", fields.at(0).c_str());
  ASSERT_STREQ("\"A", fields.at(1).c_str());

  tokenize("D (A B) C (E F) G", ' ', '(', ')', &fields);
  ASSERT_EQ(5ul, fields.size());
  ASSERT_STREQ("D", fields.at(0).c_str());
  ASSERT_STREQ("A B", fields.at(1).c_str());
  ASSERT_STREQ("E F", fields.at(3).c_str());
}

// Tests counting substrings.
TEST(SplitTest, CountSubstrings) {
  std::vector<std::string> fields;

  EXPECT_EQ(5ul, split("one,two,three,four,five", ',', &fields));
  fields.clear();
  EXPECT_EQ(1ul, split("one", ',', &fields));

  // Empty fields between commas count.
  fields.clear();
  EXPECT_EQ(5ul, split("one,,three,four,five", ',', &fields));
  fields.clear();
  EXPECT_EQ(3ul, split(",three,", ',', &fields));
  fields.clear();
  EXPECT_EQ(1ul, split("", ',', &fields));
}

// Tests comparing substrings.
TEST(SplitTest, CompareSubstrings) {
  std::vector<std::string> fields;

  split("find,middle,one", ',', &fields);
  ASSERT_EQ(3ul, fields.size());
  ASSERT_STREQ("middle", fields.at(1).c_str());
  fields.clear();

  // Empty fields between commas count.
  split("find,,middle,one", ',', &fields);
  ASSERT_EQ(4ul, fields.size());
  ASSERT_STREQ("middle", fields.at(2).c_str());
  fields.clear();
  split("", ',', &fields);
  ASSERT_EQ(1ul, fields.size());
  ASSERT_STREQ("", fields.at(0).c_str());
}

TEST(BoolTest, DecodeValid) {
  bool value;
  EXPECT_TRUE(FromString("true", &value));
  EXPECT_TRUE(value);
  EXPECT_TRUE(FromString("true,", &value));
  EXPECT_TRUE(value);
  EXPECT_TRUE(FromString("true , true", &value));
  EXPECT_TRUE(value);
  EXPECT_TRUE(FromString("true ,\n false", &value));
  EXPECT_TRUE(value);
  EXPECT_TRUE(FromString("  true  \n", &value));
  EXPECT_TRUE(value);

  EXPECT_TRUE(FromString("false", &value));
  EXPECT_FALSE(value);
  EXPECT_TRUE(FromString("  false ", &value));
  EXPECT_FALSE(value);
  EXPECT_TRUE(FromString("  false, ", &value));
  EXPECT_FALSE(value);

  EXPECT_TRUE(FromString<bool>("true\n"));
  EXPECT_FALSE(FromString<bool>("false\n"));
}

TEST(BoolTest, DecodeInvalid) {
  bool value;
  EXPECT_FALSE(FromString("True", &value));
  EXPECT_FALSE(FromString("TRUE", &value));
  EXPECT_FALSE(FromString("False", &value));
  EXPECT_FALSE(FromString("FALSE", &value));
  EXPECT_FALSE(FromString("0", &value));
  EXPECT_FALSE(FromString("1", &value));
  EXPECT_FALSE(FromString("0,", &value));
  EXPECT_FALSE(FromString("1,", &value));
  EXPECT_FALSE(FromString("1,0", &value));
  EXPECT_FALSE(FromString("1.", &value));
  EXPECT_FALSE(FromString("1.0", &value));
  EXPECT_FALSE(FromString("", &value));
  EXPECT_FALSE(FromString<bool>("false\nfalse"));
}

TEST(BoolTest, RoundTrip) {
  bool value;
  EXPECT_TRUE(FromString(ToString(true), &value));
  EXPECT_TRUE(value);
  EXPECT_TRUE(FromString(ToString(false), &value));
  EXPECT_FALSE(value);
}
}  // namespace talk_base
