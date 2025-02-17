// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

TEST(BluetoothUUIDTest, BluetoothUUID) {
  const char kValid128Bit0[] = "12345678-1234-5678-9abc-def123456789";
  const char kValid128Bit1[] = "00001101-0000-1000-8000-00805f9b34fb";
  const char kInvalid36Char0[] = "1234567-1234-5678-9abc-def123456789";
  const char kInvalid36Char1[] = "0x00001101-0000-1000-8000-00805f9b34fb";
  const char kInvalid4Char[] = "Z101";
  const char kValid16Bit[] = "0x1101";
  const char kValid32Bit[] = "00001101";

  // Valid 128-bit custom UUID.
  BluetoothUUID uuid0(kValid128Bit0);
  EXPECT_TRUE(uuid0.IsValid());
  EXPECT_EQ(BluetoothUUID::kFormat128Bit, uuid0.format());
  EXPECT_EQ(uuid0.value(), uuid0.canonical_value());

  // Valid 128-bit UUID.
  BluetoothUUID uuid1(kValid128Bit1);
  EXPECT_TRUE(uuid1.IsValid());
  EXPECT_EQ(BluetoothUUID::kFormat128Bit, uuid1.format());
  EXPECT_EQ(uuid1.value(), uuid1.canonical_value());

  EXPECT_NE(uuid0, uuid1);

  // Invalid 128-bit UUID.
  BluetoothUUID uuid2(kInvalid36Char0);
  EXPECT_FALSE(uuid2.IsValid());
  EXPECT_EQ(BluetoothUUID::kFormatInvalid, uuid2.format());
  EXPECT_TRUE(uuid2.value().empty());
  EXPECT_TRUE(uuid2.canonical_value().empty());

  // Invalid 128-bit UUID.
  BluetoothUUID uuid3(kInvalid36Char1);
  EXPECT_FALSE(uuid3.IsValid());
  EXPECT_EQ(BluetoothUUID::kFormatInvalid, uuid3.format());
  EXPECT_TRUE(uuid3.value().empty());
  EXPECT_TRUE(uuid3.canonical_value().empty());

  // Invalid 16-bit UUID.
  BluetoothUUID uuid4(kInvalid4Char);
  EXPECT_FALSE(uuid4.IsValid());
  EXPECT_EQ(BluetoothUUID::kFormatInvalid, uuid4.format());
  EXPECT_TRUE(uuid4.value().empty());
  EXPECT_TRUE(uuid4.canonical_value().empty());

  // Valid 16-bit UUID.
  BluetoothUUID uuid5(kValid16Bit);
  EXPECT_TRUE(uuid5.IsValid());
  EXPECT_EQ(BluetoothUUID::kFormat16Bit, uuid5.format());
  EXPECT_NE(uuid5.value(), uuid5.canonical_value());
  EXPECT_EQ("1101", uuid5.value());
  EXPECT_EQ(kValid128Bit1, uuid5.canonical_value());

  // Valid 32-bit UUID.
  BluetoothUUID uuid6(kValid32Bit);
  EXPECT_TRUE(uuid6.IsValid());
  EXPECT_EQ(BluetoothUUID::kFormat32Bit, uuid6.format());
  EXPECT_NE(uuid6.value(), uuid6.canonical_value());
  EXPECT_EQ("00001101", uuid6.value());
  EXPECT_EQ(kValid128Bit1, uuid6.canonical_value());

  // uuid5, uuid6, and uuid1 are equivalent.
  EXPECT_EQ(uuid5, uuid6);
  EXPECT_EQ(uuid1, uuid5);
  EXPECT_EQ(uuid1, uuid6);
}

// Verify that UUIDs are parsed case-insensitively
TEST(BluetoothUUIDTest, BluetoothUUID_CaseInsensitive) {
  const char k16Bit[] = "1abc";
  const char k32Bit[] = "00001abc";
  const char k128Bit[] = "00001abc-0000-1000-8000-00805f9b34fb";

  struct TestCase {
    const std::string input_uuid;
    const std::string expected_value;
  } test_cases[] = {
    { "1abc", k16Bit },
    { "1ABC", k16Bit },
    { "1aBc", k16Bit },
    { "00001abc", k32Bit },
    { "00001ABC", k32Bit },
    { "00001aBc", k32Bit },
    { "00001abc-0000-1000-8000-00805f9b34fb", k128Bit },
    { "00001ABC-0000-1000-8000-00805F9B34FB", k128Bit },
    { "00001aBc-0000-1000-8000-00805F9b34fB", k128Bit },
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(test_cases); ++i) {
    SCOPED_TRACE("Input UUID: " + test_cases[i].input_uuid);
    BluetoothUUID uuid(test_cases[i].input_uuid);
    EXPECT_TRUE(uuid.IsValid());
    EXPECT_EQ(test_cases[i].expected_value, uuid.value());
    EXPECT_EQ(k128Bit, uuid.canonical_value());
  }
}

}  // namespace device
