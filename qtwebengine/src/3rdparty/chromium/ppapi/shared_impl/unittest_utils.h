// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHARED_IMPL_UNITTEST_UTILS_H_
#define PPAPI_SHARED_IMPL_UNITTEST_UTILS_H_

#include "ppapi/c/pp_var.h"

namespace ppapi {

// Compares two vars for equality. This is a deep comparison (the entire graph
// is traversed recursively). The function guarantees that the topology of the
// two PP_Var graphs being compared is identical, including graphs with cycles.
// If |test_string_references| is set to true, then incoming references to
// string vars in the two graphs must be isomorphic. Otherwise only the content
// of the strings is tested for equality.
bool TestEqual(const PP_Var& expected,
               const PP_Var& actual,
               bool test_string_references);

}  // namespace ppapi

#endif  // PPAPI_SHARED_IMPL_UNITTEST_UTILS_H_
