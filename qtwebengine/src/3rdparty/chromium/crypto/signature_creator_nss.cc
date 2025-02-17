// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crypto/signature_creator.h"

#include <cryptohi.h>
#include <keyhi.h>
#include <stdlib.h>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "crypto/nss_util.h"
#include "crypto/rsa_private_key.h"

namespace crypto {

SignatureCreator::~SignatureCreator() {
  if (sign_context_) {
    SGN_DestroyContext(sign_context_, PR_TRUE);
    sign_context_ = NULL;
  }
}

// static
SignatureCreator* SignatureCreator::Create(RSAPrivateKey* key) {
  scoped_ptr<SignatureCreator> result(new SignatureCreator);
  result->key_ = key;

  result->sign_context_ = SGN_NewContext(SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION,
      key->key());
  if (!result->sign_context_) {
    NOTREACHED();
    return NULL;
  }

  SECStatus rv = SGN_Begin(result->sign_context_);
  if (rv != SECSuccess) {
    NOTREACHED();
    return NULL;
  }

  return result.release();
}

// static
bool SignatureCreator::Sign(RSAPrivateKey* key,
                            const uint8* data,
                            int data_len,
                            std::vector<uint8>* signature) {
  SECItem data_item;
  data_item.type = siBuffer;
  data_item.data = const_cast<unsigned char*>(data);
  data_item.len = data_len;

  SECItem signature_item;
  SECStatus rv = SGN_Digest(key->key(), SEC_OID_SHA1, &signature_item,
                            &data_item);
  if (rv != SECSuccess) {
    NOTREACHED();
    return false;
  }
  signature->assign(signature_item.data,
                    signature_item.data + signature_item.len);
  SECITEM_FreeItem(&signature_item, PR_FALSE);
  return true;
}

bool SignatureCreator::Update(const uint8* data_part, int data_part_len) {
  SECStatus rv = SGN_Update(sign_context_, data_part, data_part_len);
  if (rv != SECSuccess) {
    NOTREACHED();
    return false;
  }

  return true;
}

bool SignatureCreator::Final(std::vector<uint8>* signature) {
  SECItem signature_item;
  SECStatus rv = SGN_End(sign_context_, &signature_item);
  if (rv != SECSuccess) {
    return false;
  }
  signature->assign(signature_item.data,
                    signature_item.data + signature_item.len);
  SECITEM_FreeItem(&signature_item, PR_FALSE);
  return true;
}

SignatureCreator::SignatureCreator()
    : key_(NULL),
      sign_context_(NULL) {
  EnsureNSSInit();
}

}  // namespace crypto
