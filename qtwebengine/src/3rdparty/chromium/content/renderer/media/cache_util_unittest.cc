// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/cache_util.h"

#include <string>

#include "base/format_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebURLResponse.h"

using blink::WebString;
using blink::WebURLResponse;

namespace content {

// Inputs & expected output for GetReasonsForUncacheability.
struct GRFUTestCase {
  WebURLResponse::HTTPVersion version;
  int status_code;
  const char* headers;
  uint32 expected_reasons;
};

// Create a new WebURLResponse object.
static WebURLResponse CreateResponse(const GRFUTestCase& test) {
  WebURLResponse response;
  response.initialize();
  response.setHTTPVersion(test.version);
  response.setHTTPStatusCode(test.status_code);
  std::vector<std::string> lines;
  Tokenize(test.headers, "\n", &lines);
  for (size_t i = 0; i < lines.size(); ++i) {
    size_t colon = lines[i].find(": ");
    response.addHTTPHeaderField(
        WebString::fromUTF8(lines[i].substr(0, colon)),
        WebString::fromUTF8(lines[i].substr(colon + 2)));
  }
  return response;
}

TEST(CacheUtilTest, GetReasonsForUncacheability) {
  enum { kNoReasons = 0 };

  const GRFUTestCase tests[] = {
    {
      WebURLResponse::HTTP_1_1, 206, "ETag: 'fooblort'", kNoReasons
    },
    {
      WebURLResponse::HTTP_1_1, 206, "", kNoStrongValidatorOnPartialResponse
    },
    {
      WebURLResponse::HTTP_1_0, 206, "",
      kPre11PartialResponse | kNoStrongValidatorOnPartialResponse
    },
    {
      WebURLResponse::HTTP_1_1, 200, "cache-control: max-Age=42", kShortMaxAge
    },
    {
      WebURLResponse::HTTP_1_1, 200, "cache-control: max-Age=4200", kNoReasons
    },
    {
      WebURLResponse::HTTP_1_1, 200,
      "Date: Tue, 22 May 2012 23:46:08 GMT\n"
      "Expires: Tue, 22 May 2012 23:56:08 GMT", kExpiresTooSoon
    },
    {
      WebURLResponse::HTTP_1_1, 200, "cache-control: must-revalidate",
      kHasMustRevalidate
    },
    {
      WebURLResponse::HTTP_1_1, 200, "cache-control: no-cache", kNoCache
    },
    {
      WebURLResponse::HTTP_1_1, 200, "cache-control: no-store", kNoStore
    },
    {
      WebURLResponse::HTTP_1_1, 200,
      "cache-control: no-cache\ncache-control: no-store", kNoCache | kNoStore
    },
  };
  for (size_t i = 0; i < arraysize(tests); ++i) {
    SCOPED_TRACE(base::StringPrintf("case: %" PRIuS
                                    ", version: %d, code: %d, headers: %s",
                                    i, tests[i].version, tests[i].status_code,
                                    tests[i].headers));
    EXPECT_EQ(GetReasonsForUncacheability(CreateResponse(tests[i])),
              tests[i].expected_reasons);
  }
}

}  // namespace content
