// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_NET_ADDRESS_RESOURCE_H_
#define PPAPI_PROXY_NET_ADDRESS_RESOURCE_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ppapi/c/private/ppb_net_address_private.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/thunk/ppb_net_address_api.h"

namespace ppapi {
namespace proxy {

class PPAPI_PROXY_EXPORT NetAddressResource : public PluginResource,
                                              public thunk::PPB_NetAddress_API {
 public:
  NetAddressResource(Connection connection,
                     PP_Instance instance,
                     const PP_NetAddress_IPv4& ipv4_addr);
  NetAddressResource(Connection connection,
                     PP_Instance instance,
                     const PP_NetAddress_IPv6& ipv6_addr);
  NetAddressResource(Connection connection,
                     PP_Instance instance,
                     const PP_NetAddress_Private& private_addr);

  virtual ~NetAddressResource();

  // PluginResource implementation.
  virtual thunk::PPB_NetAddress_API* AsPPB_NetAddress_API() OVERRIDE;

  // PPB_NetAddress_API implementation.
  virtual PP_NetAddress_Family GetFamily() OVERRIDE;
  virtual PP_Var DescribeAsString(PP_Bool include_port) OVERRIDE;
  virtual PP_Bool DescribeAsIPv4Address(
      PP_NetAddress_IPv4* ipv4_addr) OVERRIDE;
  virtual PP_Bool DescribeAsIPv6Address(
      PP_NetAddress_IPv6* ipv6_addr) OVERRIDE;
  virtual const PP_NetAddress_Private& GetNetAddressPrivate() OVERRIDE;

 private:
  // TODO(yzshen): Refactor the code so that PPB_NetAddress resource doesn't
  // use PP_NetAddress_Private as storage type.
  PP_NetAddress_Private address_;

  DISALLOW_COPY_AND_ASSIGN(NetAddressResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_NET_ADDRESS_RESOURCE_H_
