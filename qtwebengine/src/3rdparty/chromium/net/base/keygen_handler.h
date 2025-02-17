// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_KEYGEN_HANDLER_H_
#define NET_BASE_KEYGEN_HANDLER_H_

#include <string>

#include "base/callback_forward.h"
#include "base/memory/scoped_ptr.h"
#include "build/build_config.h"
#include "net/base/net_export.h"
#include "url/gurl.h"

namespace crypto {
class NSSCryptoModuleDelegate;
}

namespace net {

// This class handles keypair generation for generating client
// certificates via the <keygen> tag.
// <http://dev.w3.org/html5/spec/Overview.html#the-keygen-element>
// <https://developer.mozilla.org/En/HTML/HTML_Extensions/KEYGEN_Tag>

class NET_EXPORT KeygenHandler {
 public:
  // Creates a handler that will generate a key with the given key size and
  // incorporate the |challenge| into the Netscape SPKAC structure. The request
  // for the key originated from |url|.
  KeygenHandler(int key_size_in_bits,
                const std::string& challenge,
                const GURL& url);
  ~KeygenHandler();

  // Actually generates the key-pair and the cert request (SPKAC), and returns
  // a base64-encoded string suitable for use as the form value of <keygen>.
  std::string GenKeyAndSignChallenge();

  // Exposed only for unit tests.
  void set_stores_key(bool store) { stores_key_ = store;}

#if defined(USE_NSS)
  // Register the delegate to be used to get the token to store the key in, and
  // to get the password if the token is unauthenticated.
  // GenKeyAndSignChallenge runs on a worker thread, so using a blocking
  // password callback is okay here.
  void set_crypto_module_delegate(
      scoped_ptr<crypto::NSSCryptoModuleDelegate> delegate);
#endif  // defined(USE_NSS)

 private:
  int key_size_in_bits_;  // key size in bits (usually 2048)
  std::string challenge_;  // challenge string sent by server
  GURL url_;  // the URL that requested the key
  bool stores_key_;  // should the generated key-pair be stored persistently?
#if defined(USE_NSS)
  // The callback for requesting a password to the PKCS#11 token.
  scoped_ptr<crypto::NSSCryptoModuleDelegate> crypto_module_delegate_;
#endif  // defined(USE_NSS)
};

}  // namespace net

#endif  // NET_BASE_KEYGEN_HANDLER_H_
