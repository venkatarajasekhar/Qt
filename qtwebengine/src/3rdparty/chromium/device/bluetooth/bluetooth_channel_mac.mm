// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_channel_mac.h"

#import <IOBluetooth/IOBluetooth.h>

#include "base/logging.h"
#include "device/bluetooth/bluetooth_device_mac.h"

namespace device {

BluetoothChannelMac::BluetoothChannelMac() : socket_(NULL) {
}

BluetoothChannelMac::~BluetoothChannelMac() {
}

void BluetoothChannelMac::SetSocket(BluetoothSocketMac* socket) {
  DCHECK(!socket_);
  socket_ = socket;
}

std::string BluetoothChannelMac::GetDeviceAddress() {
  return BluetoothDeviceMac::GetDeviceAddress(GetDevice());
}

}  // namespace device
