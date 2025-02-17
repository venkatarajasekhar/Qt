// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_CERT_CERT_VERIFY_PROC_MAC_H_
#define NET_CERT_CERT_VERIFY_PROC_MAC_H_

#include "net/cert/cert_verify_proc.h"

namespace net {

// Performs certificate path construction and validation using OS X's
// Security.framework.
class CertVerifyProcMac : public CertVerifyProc {
 public:
  CertVerifyProcMac();

  virtual bool SupportsAdditionalTrustAnchors() const OVERRIDE;

 protected:
  virtual ~CertVerifyProcMac();

 private:
  virtual int VerifyInternal(X509Certificate* cert,
                             const std::string& hostname,
                             int flags,
                             CRLSet* crl_set,
                             const CertificateList& additional_trust_anchors,
                             CertVerifyResult* verify_result) OVERRIDE;
};

}  // namespace net

#endif  // NET_CERT_CERT_VERIFY_PROC_MAC_H_
