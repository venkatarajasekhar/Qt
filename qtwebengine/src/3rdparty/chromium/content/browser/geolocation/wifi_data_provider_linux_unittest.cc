// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/geolocation/wifi_data_provider_linux.h"

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "dbus/message.h"
#include "dbus/mock_bus.h"
#include "dbus/mock_object_proxy.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Unused;

namespace content {

class GeolocationWifiDataProviderLinuxTest : public testing::Test {
  virtual void SetUp() {
    // Create a mock bus.
    dbus::Bus::Options options;
    options.bus_type = dbus::Bus::SYSTEM;
    mock_bus_ = new dbus::MockBus(options);

    // Create a mock proxy that behaves as NetworkManager.
    mock_network_manager_proxy_ =
        new dbus::MockObjectProxy(
            mock_bus_.get(),
            "org.freedesktop.NetworkManager",
            dbus::ObjectPath("/org/freedesktop/NetworkManager"));
    // Set an expectation so mock_network_manager_proxy_'s
    // CallMethodAndBlock() will use CreateNetworkManagerProxyResponse()
    // to return responses.
    EXPECT_CALL(*mock_network_manager_proxy_.get(),
                MockCallMethodAndBlock(_, _))
        .WillRepeatedly(Invoke(this,
                               &GeolocationWifiDataProviderLinuxTest::
                                   CreateNetworkManagerProxyResponse));

    // Create a mock proxy that behaves as NetworkManager/Devices/0.
    mock_device_proxy_ =
        new dbus::MockObjectProxy(
            mock_bus_.get(),
            "org.freedesktop.NetworkManager",
            dbus::ObjectPath("/org/freedesktop/NetworkManager/Devices/0"));
    EXPECT_CALL(*mock_device_proxy_.get(), MockCallMethodAndBlock(_, _))
        .WillRepeatedly(Invoke(
             this,
             &GeolocationWifiDataProviderLinuxTest::CreateDeviceProxyResponse));

    // Create a mock proxy that behaves as NetworkManager/AccessPoint/0.
    mock_access_point_proxy_ =
        new dbus::MockObjectProxy(
            mock_bus_.get(),
            "org.freedesktop.NetworkManager",
            dbus::ObjectPath("/org/freedesktop/NetworkManager/AccessPoint/0"));
    EXPECT_CALL(*mock_access_point_proxy_.get(), MockCallMethodAndBlock(_, _))
        .WillRepeatedly(Invoke(this,
                               &GeolocationWifiDataProviderLinuxTest::
                                   CreateAccessPointProxyResponse));

    // Set an expectation so mock_bus_'s GetObjectProxy() for the given
    // service name and the object path will return
    // mock_network_manager_proxy_.
    EXPECT_CALL(
        *mock_bus_.get(),
        GetObjectProxy("org.freedesktop.NetworkManager",
                       dbus::ObjectPath("/org/freedesktop/NetworkManager")))
        .WillOnce(Return(mock_network_manager_proxy_.get()));
    // Likewise, set an expectation for mock_device_proxy_.
    EXPECT_CALL(
        *mock_bus_.get(),
        GetObjectProxy(
            "org.freedesktop.NetworkManager",
            dbus::ObjectPath("/org/freedesktop/NetworkManager/Devices/0")))
        .WillOnce(Return(mock_device_proxy_.get()))
        .WillOnce(Return(mock_device_proxy_.get()));
    // Likewise, set an expectation for mock_access_point_proxy_.
    EXPECT_CALL(
        *mock_bus_.get(),
        GetObjectProxy(
            "org.freedesktop.NetworkManager",
            dbus::ObjectPath("/org/freedesktop/NetworkManager/AccessPoint/0")))
        .WillOnce(Return(mock_access_point_proxy_.get()));

    // ShutdownAndBlock() should be called.
    EXPECT_CALL(*mock_bus_.get(), ShutdownAndBlock()).WillOnce(Return());

    // Create the wlan API with the mock bus object injected.
    wifi_provider_linux_ = new WifiDataProviderLinux;
    wlan_api_.reset(
        wifi_provider_linux_->NewWlanApiForTesting(mock_bus_.get()));
    ASSERT_TRUE(wlan_api_.get());
  }

 protected:
  // DeviceDataProviderImplBase, a super class of WifiDataProviderLinux,
  // requires a message loop to be present. message_loop_ is defined here,
  // as it should outlive wifi_provider_linux_.
  base::MessageLoop message_loop_;
  scoped_refptr<dbus::MockBus> mock_bus_;
  scoped_refptr<dbus::MockObjectProxy> mock_network_manager_proxy_;
  scoped_refptr<dbus::MockObjectProxy> mock_access_point_proxy_;
  scoped_refptr<dbus::MockObjectProxy> mock_device_proxy_;
  scoped_refptr<WifiDataProviderLinux>  wifi_provider_linux_;
  scoped_ptr<WifiDataProviderCommon::WlanApiInterface> wlan_api_;

 private:
  // Creates a response for |mock_network_manager_proxy_|.
  dbus::Response* CreateNetworkManagerProxyResponse(
      dbus::MethodCall* method_call,
      Unused) {
    if (method_call->GetInterface() == "org.freedesktop.NetworkManager" &&
        method_call->GetMember() == "GetDevices") {
      // The list of devices is asked. Return the object path.
      std::vector<dbus::ObjectPath> object_paths;
      object_paths.push_back(
          dbus::ObjectPath("/org/freedesktop/NetworkManager/Devices/0"));

      scoped_ptr<dbus::Response> response = dbus::Response::CreateEmpty();
      dbus::MessageWriter writer(response.get());
      writer.AppendArrayOfObjectPaths(object_paths);
      return response.release();
    }

    LOG(ERROR) << "Unexpected method call: " << method_call->ToString();
    return NULL;
  }

  // Creates a response for |mock_device_proxy_|.
  dbus::Response* CreateDeviceProxyResponse(dbus::MethodCall* method_call,
                                            Unused) {
    if (method_call->GetInterface() == DBUS_INTERFACE_PROPERTIES &&
        method_call->GetMember() == "Get") {
      dbus::MessageReader reader(method_call);
      std::string interface_name;
      std::string property_name;
      if (reader.PopString(&interface_name) &&
          reader.PopString(&property_name)) {
        // The device type is asked. Respond that the device type is wifi.
        scoped_ptr<dbus::Response> response = dbus::Response::CreateEmpty();
        dbus::MessageWriter writer(response.get());
        // This matches NM_DEVICE_TYPE_WIFI in wifi_data_provider_linux.cc.
        const int kDeviceTypeWifi = 2;
        writer.AppendVariantOfUint32(kDeviceTypeWifi);
        return response.release();
      }
    } else if (method_call->GetInterface() ==
               "org.freedesktop.NetworkManager.Device.Wireless" &&
               method_call->GetMember() == "GetAccessPoints") {
      // The list of access points is asked. Return the object path.
      scoped_ptr<dbus::Response> response = dbus::Response::CreateEmpty();
      dbus::MessageWriter writer(response.get());
      std::vector<dbus::ObjectPath> object_paths;
      object_paths.push_back(
          dbus::ObjectPath("/org/freedesktop/NetworkManager/AccessPoint/0"));
      writer.AppendArrayOfObjectPaths(object_paths);
      return response.release();
    }

    LOG(ERROR) << "Unexpected method call: " << method_call->ToString();
    return NULL;
  }


  // Creates a response for |mock_access_point_proxy_|.
  dbus::Response* CreateAccessPointProxyResponse(dbus::MethodCall* method_call,
                                                 Unused) {
    if (method_call->GetInterface() == DBUS_INTERFACE_PROPERTIES &&
        method_call->GetMember() == "Get") {
      dbus::MessageReader reader(method_call);

      std::string interface_name;
      std::string property_name;
      if (reader.PopString(&interface_name) &&
          reader.PopString(&property_name)) {
        scoped_ptr<dbus::Response> response = dbus::Response::CreateEmpty();
        dbus::MessageWriter writer(response.get());

        if (property_name == "Ssid") {
          const uint8 kSsid[] = {0x74, 0x65, 0x73, 0x74};  // "test"
          dbus::MessageWriter variant_writer(response.get());
          writer.OpenVariant("ay", &variant_writer);
          variant_writer.AppendArrayOfBytes(kSsid, arraysize(kSsid));
          writer.CloseContainer(&variant_writer);
        } else if (property_name == "HwAddress") {
          // This will be converted to "00-11-22-33-44-55".
          const std::string kMacAddress = "00:11:22:33:44:55";
          writer.AppendVariantOfString(kMacAddress);
        } else if (property_name == "Strength") {
          // This will be converted to -50.
          const uint8 kStrength = 100;
          writer.AppendVariantOfByte(kStrength);
        } else if (property_name == "Frequency") {
          // This will be converted to channel 4.
          const uint32 kFrequency = 2427;
          writer.AppendVariantOfUint32(kFrequency);
        }
        return response.release();
      }
    }

    LOG(ERROR) << "Unexpected method call: " << method_call->ToString();
    return NULL;
  }
};

TEST_F(GeolocationWifiDataProviderLinuxTest, GetAccessPointData) {
  WifiData::AccessPointDataSet access_point_data_set;
  ASSERT_TRUE(wlan_api_->GetAccessPointData(&access_point_data_set));

  ASSERT_EQ(1U, access_point_data_set.size());
  AccessPointData access_point_data = *access_point_data_set.begin();

  // Check the contents of the access point data.
  // The expected values come from CreateAccessPointProxyResponse() above.
  EXPECT_EQ("test", base::UTF16ToUTF8(access_point_data.ssid));
  EXPECT_EQ("00-11-22-33-44-55",
            base::UTF16ToUTF8(access_point_data.mac_address));
  EXPECT_EQ(-50, access_point_data.radio_signal_strength);
  EXPECT_EQ(4, access_point_data.channel);
}

}  // namespace content
