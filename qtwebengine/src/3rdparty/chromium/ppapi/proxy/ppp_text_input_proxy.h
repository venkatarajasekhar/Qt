// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_PPP_TEXT_INPUT_PROXY_H_
#define PPAPI_PROXY_PPP_TEXT_INPUT_PROXY_H_

#include "base/compiler_specific.h"
#include "ppapi/c/dev/ppp_text_input_dev.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/proxy/interface_proxy.h"

namespace ppapi {
namespace proxy {

class PPP_TextInput_Proxy : public InterfaceProxy {
 public:
  PPP_TextInput_Proxy(Dispatcher* dispatcher);
  virtual ~PPP_TextInput_Proxy();

  static const PPP_TextInput_Dev* GetProxyInterface();

  // InterfaceProxy implementation.
  virtual bool OnMessageReceived(const IPC::Message& msg) OVERRIDE;

 private:
  // Message handlers.
  void OnMsgRequestSurroundingText(PP_Instance instance,
                                   uint32_t desired_number_of_characters);

  // When this proxy is in the plugin side, this value caches the interface
  // pointer so we don't have to retrieve it from the dispatcher each time.
  // In the host, this value is always NULL.
  const PPP_TextInput_Dev* ppp_text_input_impl_;

  DISALLOW_COPY_AND_ASSIGN(PPP_TextInput_Proxy);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_PPP_TEXT_INPUT_PROXY_H_
