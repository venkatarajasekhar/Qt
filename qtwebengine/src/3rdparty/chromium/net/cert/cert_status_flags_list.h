// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is the list of CertStatus flags and their values.
//
// Defines the values using a macro CERT_STATUS_FLAG,
// so it can be expanded differently in some places

// The possible status bits for CertStatus.
// Bits 0 to 15 are for errors.
CERT_STATUS_FLAG(COMMON_NAME_INVALID, 1 << 0);
CERT_STATUS_FLAG(DATE_INVALID, 1 << 1);
CERT_STATUS_FLAG(AUTHORITY_INVALID, 1 << 2);
// 1 << 3 is reserved for ERR_CERT_CONTAINS_ERRORS (not useful with WinHTTP).
CERT_STATUS_FLAG(NO_REVOCATION_MECHANISM, 1 << 4);
CERT_STATUS_FLAG(UNABLE_TO_CHECK_REVOCATION, 1 << 5);
CERT_STATUS_FLAG(REVOKED, 1 << 6);
CERT_STATUS_FLAG(INVALID, 1 << 7);
CERT_STATUS_FLAG(WEAK_SIGNATURE_ALGORITHM, 1 << 8);
// 1 << 9 was used for CERT_STATUS_NOT_IN_DNS
CERT_STATUS_FLAG(NON_UNIQUE_NAME, 1 << 10);
CERT_STATUS_FLAG(WEAK_KEY, 1 << 11);
// 1 << 12 was used for CERT_STATUS_WEAK_DH_KEY
CERT_STATUS_FLAG(PINNED_KEY_MISSING, 1 << 13);
CERT_STATUS_FLAG(NAME_CONSTRAINT_VIOLATION, 1 << 14);

// Bits 16 to 31 are for non-error statuses.
CERT_STATUS_FLAG(IS_EV, 1 << 16);
CERT_STATUS_FLAG(REV_CHECKING_ENABLED, 1 << 17);
// Bit 18 was CERT_STATUS_IS_DNSSEC
