// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_FLASH_CLIPBOARD_RESOURCE_H_
#define PPAPI_PROXY_FLASH_CLIPBOARD_RESOURCE_H_

#include "ppapi/proxy/connection.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/shared_impl/flash_clipboard_format_registry.h"
#include "ppapi/thunk/ppb_flash_clipboard_api.h"

namespace ppapi {
namespace proxy {

class FlashClipboardResource
    : public PluginResource,
      public NON_EXPORTED_BASE(thunk::PPB_Flash_Clipboard_API) {
 public:
  FlashClipboardResource(Connection connection, PP_Instance instance);
  virtual ~FlashClipboardResource();

  // Resource implementation.
  virtual thunk::PPB_Flash_Clipboard_API* AsPPB_Flash_Clipboard_API() OVERRIDE;

  // PPB_Flash_Clipboard_API implementation.
  virtual uint32_t RegisterCustomFormat(PP_Instance instance,
                                        const char* format_name) OVERRIDE;
  virtual PP_Bool IsFormatAvailable(PP_Instance instance,
                                    PP_Flash_Clipboard_Type clipboard_type,
                                    uint32_t format) OVERRIDE;
  virtual PP_Var ReadData(PP_Instance instance,
                          PP_Flash_Clipboard_Type clipboard_type,
                          uint32_t format) OVERRIDE;
  virtual int32_t WriteData(PP_Instance instance,
                            PP_Flash_Clipboard_Type clipboard_type,
                            uint32_t data_item_count,
                            const uint32_t formats[],
                            const PP_Var data_items[]) OVERRIDE;
  virtual PP_Bool GetSequenceNumber(
      PP_Instance instance,
      PP_Flash_Clipboard_Type clipboard_type,
      uint64_t* sequence_number) OVERRIDE;

 private:
  FlashClipboardFormatRegistry clipboard_formats_;

  DISALLOW_COPY_AND_ASSIGN(FlashClipboardResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_FLASH_CLIPBOARD_RESOURCE_H_
