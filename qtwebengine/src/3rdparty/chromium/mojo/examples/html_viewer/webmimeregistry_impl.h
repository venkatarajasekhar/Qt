// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EXAMPLES_HTML_VIEWER_WEBMIMEREGISTRY_IMPL_H_
#define MOJO_EXAMPLES_HTML_VIEWER_WEBMIMEREGISTRY_IMPL_H_

#include "base/compiler_specific.h"
#include "third_party/WebKit/public/platform/WebMimeRegistry.h"

namespace mojo {
namespace examples {

class WebMimeRegistryImpl : public blink::WebMimeRegistry {
 public:
  WebMimeRegistryImpl() {}
  virtual ~WebMimeRegistryImpl() {}

  // WebMimeRegistry methods:
  virtual blink::WebMimeRegistry::SupportsType supportsMIMEType(
      const blink::WebString& mime_type);
  virtual blink::WebMimeRegistry::SupportsType supportsImageMIMEType(
      const blink::WebString& mime_type);
  virtual blink::WebMimeRegistry::SupportsType supportsJavaScriptMIMEType(
      const blink::WebString& mime_type);
  virtual blink::WebMimeRegistry::SupportsType supportsMediaMIMEType(
      const blink::WebString& mime_type,
      const blink::WebString& codecs,
      const blink::WebString& key_system);
  virtual bool supportsMediaSourceMIMEType(
      const blink::WebString& mime_type,
      const blink::WebString& codecs);
  virtual bool supportsEncryptedMediaMIMEType(
      const blink::WebString& key_system,
      const blink::WebString& mime_type,
      const blink::WebString& codecs);
  virtual blink::WebMimeRegistry::SupportsType supportsNonImageMIMEType(
      const blink::WebString& mime_type);
  virtual blink::WebString mimeTypeForExtension(
      const blink::WebString& extension);
  virtual blink::WebString wellKnownMimeTypeForExtension(
      const blink::WebString& extension);
  virtual blink::WebString mimeTypeFromFile(
      const blink::WebString& path);
};

}  // namespace examples
}  // namespace mojo

#endif  // MOJO_EXAMPLES_HTML_VIEWER_WEBMIMEREGISTRY_IMPL_H_
