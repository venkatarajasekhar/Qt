// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_URL_REQUEST_INFO_RESOURCE_H_
#define PPAPI_PROXY_URL_REQUEST_INFO_RESOURCE_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/url_request_info_data.h"
#include "ppapi/thunk/ppb_url_request_info_api.h"

namespace ppapi {
namespace proxy {

class PPAPI_PROXY_EXPORT URLRequestInfoResource
    : public PluginResource,
      public thunk::PPB_URLRequestInfo_API {
 public:
  URLRequestInfoResource(Connection connection, PP_Instance instance,
                         const URLRequestInfoData& data);
  virtual ~URLRequestInfoResource();

  // Resource overrides.
  virtual thunk::PPB_URLRequestInfo_API* AsPPB_URLRequestInfo_API() OVERRIDE;

  // PPB_URLRequestInfo_API implementation.
  virtual PP_Bool SetProperty(PP_URLRequestProperty property,
                              PP_Var var) OVERRIDE;
  virtual PP_Bool AppendDataToBody(const void* data, uint32_t len) OVERRIDE;
  virtual PP_Bool AppendFileToBody(
      PP_Resource file_ref,
      int64_t start_offset,
      int64_t number_of_bytes,
      PP_Time expected_last_modified_time) OVERRIDE;
  virtual const URLRequestInfoData& GetData() const OVERRIDE;

  bool SetUndefinedProperty(PP_URLRequestProperty property);
  bool SetBooleanProperty(PP_URLRequestProperty property, bool value);
  bool SetIntegerProperty(PP_URLRequestProperty property, int32_t value);
  bool SetStringProperty(PP_URLRequestProperty property,
                         const std::string& value);

 private:
  URLRequestInfoData data_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestInfoResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_URL_REQUEST_INFO_RESOURCE_H_
