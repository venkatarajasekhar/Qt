/*
 * Copyright (c) 2011 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef PPAPI_NATIVE_CLIENT_SRC_UNTRUSTED_PNACL_IRT_SHIM_SHIM_PPAPI_H_
#define PPAPI_NATIVE_CLIENT_SRC_UNTRUSTED_PNACL_IRT_SHIM_SHIM_PPAPI_H_ 1

#include <stddef.h>
#include "native_client/src/untrusted/irt/irt.h"

/*
 * Remembers the IRT's true interface query function.
 */
extern TYPE_nacl_irt_query __pnacl_real_irt_query_func;

/*
 * Provides a wrapped query function.
 */
size_t __pnacl_wrap_irt_query_func(const char *interface_ident,
                                   void *table, size_t tablesize);

#endif  /* PPAPI_NATIVE_CLIENT_SRC_UNTRUSTED_PNACL_IRT_SHIM_SHIM_PPAPI_H_ */
