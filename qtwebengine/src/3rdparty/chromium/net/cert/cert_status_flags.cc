// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/cert_status_flags.h"

#include "base/logging.h"
#include "net/base/net_errors.h"

namespace net {

bool IsCertStatusMinorError(CertStatus cert_status) {
  static const CertStatus kMinorErrors =
      CERT_STATUS_UNABLE_TO_CHECK_REVOCATION |
      CERT_STATUS_NO_REVOCATION_MECHANISM;
  cert_status &= CERT_STATUS_ALL_ERRORS;
  return cert_status != 0 && (cert_status & ~kMinorErrors) == 0;
}

CertStatus MapNetErrorToCertStatus(int error) {
  switch (error) {
    case ERR_CERT_COMMON_NAME_INVALID:
      return CERT_STATUS_COMMON_NAME_INVALID;
    case ERR_CERT_DATE_INVALID:
      return CERT_STATUS_DATE_INVALID;
    case ERR_CERT_AUTHORITY_INVALID:
      return CERT_STATUS_AUTHORITY_INVALID;
    case ERR_CERT_NO_REVOCATION_MECHANISM:
      return CERT_STATUS_NO_REVOCATION_MECHANISM;
    case ERR_CERT_UNABLE_TO_CHECK_REVOCATION:
      return CERT_STATUS_UNABLE_TO_CHECK_REVOCATION;
    case ERR_CERT_REVOKED:
      return CERT_STATUS_REVOKED;
    // We added the ERR_CERT_CONTAINS_ERRORS error code when we were using
    // WinInet, but we never figured out how it differs from ERR_CERT_INVALID.
    // We should not use ERR_CERT_CONTAINS_ERRORS in new code.
    case ERR_CERT_CONTAINS_ERRORS:
      NOTREACHED();
      // Falls through.
    case ERR_CERT_INVALID:
      return CERT_STATUS_INVALID;
    case ERR_CERT_WEAK_SIGNATURE_ALGORITHM:
      return CERT_STATUS_WEAK_SIGNATURE_ALGORITHM;
    case ERR_CERT_NON_UNIQUE_NAME:
      return CERT_STATUS_NON_UNIQUE_NAME;
    case ERR_CERT_WEAK_KEY:
      return CERT_STATUS_WEAK_KEY;
    case ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN:
      return CERT_STATUS_PINNED_KEY_MISSING;
    case ERR_CERT_NAME_CONSTRAINT_VIOLATION:
      return CERT_STATUS_NAME_CONSTRAINT_VIOLATION;
    default:
      return 0;
  }
}

int MapCertStatusToNetError(CertStatus cert_status) {
  // A certificate may have multiple errors.  We report the most
  // serious error.

  // Unrecoverable errors
  if (cert_status & CERT_STATUS_REVOKED)
    return ERR_CERT_REVOKED;
  if (cert_status & CERT_STATUS_INVALID)
    return ERR_CERT_INVALID;
  if (cert_status & CERT_STATUS_PINNED_KEY_MISSING)
    return ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN;

  // Recoverable errors
  if (cert_status & CERT_STATUS_AUTHORITY_INVALID)
    return ERR_CERT_AUTHORITY_INVALID;
  if (cert_status & CERT_STATUS_COMMON_NAME_INVALID)
    return ERR_CERT_COMMON_NAME_INVALID;
  // CERT_STATUS_NON_UNIQUE_NAME is intentionally not mapped to an error.
  // It is treated as just a warning and used to degrade the SSL UI.
  if (cert_status & CERT_STATUS_NAME_CONSTRAINT_VIOLATION)
    return ERR_CERT_NAME_CONSTRAINT_VIOLATION;
  if (cert_status & CERT_STATUS_WEAK_SIGNATURE_ALGORITHM)
    return ERR_CERT_WEAK_SIGNATURE_ALGORITHM;
  if (cert_status & CERT_STATUS_WEAK_KEY)
    return ERR_CERT_WEAK_KEY;
  if (cert_status & CERT_STATUS_DATE_INVALID)
    return ERR_CERT_DATE_INVALID;

  // Unknown status.  Give it the benefit of the doubt.
  if (cert_status & CERT_STATUS_UNABLE_TO_CHECK_REVOCATION)
    return ERR_CERT_UNABLE_TO_CHECK_REVOCATION;
  if (cert_status & CERT_STATUS_NO_REVOCATION_MECHANISM)
    return ERR_CERT_NO_REVOCATION_MECHANISM;

  NOTREACHED();
  return ERR_UNEXPECTED;
}

}  // namespace net
