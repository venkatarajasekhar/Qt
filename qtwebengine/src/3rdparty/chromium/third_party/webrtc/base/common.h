/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_BASE_COMMON_H_  // NOLINT
#define WEBRTC_BASE_COMMON_H_

#include "webrtc/base/basictypes.h"
#include "webrtc/base/constructormagic.h"

#if defined(_MSC_VER)
// warning C4355: 'this' : used in base member initializer list
#pragma warning(disable:4355)
#endif

//////////////////////////////////////////////////////////////////////
// General Utilities
//////////////////////////////////////////////////////////////////////

#ifndef RTC_UNUSED
#define RTC_UNUSED(x) RtcUnused(static_cast<const void*>(&x))
#define RTC_UNUSED2(x, y) RtcUnused(static_cast<const void*>(&x)); \
    RtcUnused(static_cast<const void*>(&y))
#define RTC_UNUSED3(x, y, z) RtcUnused(static_cast<const void*>(&x)); \
    RtcUnused(static_cast<const void*>(&y)); \
    RtcUnused(static_cast<const void*>(&z))
#define RTC_UNUSED4(x, y, z, a) RtcUnused(static_cast<const void*>(&x)); \
    RtcUnused(static_cast<const void*>(&y)); \
    RtcUnused(static_cast<const void*>(&z)); \
    RtcUnused(static_cast<const void*>(&a))
#define RTC_UNUSED5(x, y, z, a, b) RtcUnused(static_cast<const void*>(&x)); \
    RtcUnused(static_cast<const void*>(&y)); \
    RtcUnused(static_cast<const void*>(&z)); \
    RtcUnused(static_cast<const void*>(&a)); \
    RtcUnused(static_cast<const void*>(&b))
inline void RtcUnused(const void*) {}
#endif  // RTC_UNUSED

#if !defined(WEBRTC_WIN)

#ifndef strnicmp
#define strnicmp(x, y, n) strncasecmp(x, y, n)
#endif

#ifndef stricmp
#define stricmp(x, y) strcasecmp(x, y)
#endif

// TODO(fbarchard): Remove this. std::max should be used everywhere in the code.
// NOMINMAX must be defined where we include <windows.h>.
#define stdmax(x, y) std::max(x, y)
#else
#define stdmax(x, y) rtc::_max(x, y)
#endif

#define ARRAY_SIZE(x) (static_cast<int>(sizeof(x) / sizeof(x[0])))

/////////////////////////////////////////////////////////////////////////////
// Assertions
/////////////////////////////////////////////////////////////////////////////

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG _DEBUG
#endif  // !defined(ENABLE_DEBUG)

// Even for release builds, allow for the override of LogAssert. Though no
// macro is provided, this can still be used for explicit runtime asserts
// and allow applications to override the assert behavior.

namespace rtc {


// If a debugger is attached, triggers a debugger breakpoint. If a debugger is
// not attached, forces program termination.
void Break();

// LogAssert writes information about an assertion to the log. It's called by
// Assert (and from the ASSERT macro in debug mode) before any other action
// is taken (e.g. breaking the debugger, abort()ing, etc.).
void LogAssert(const char* function, const char* file, int line,
               const char* expression);

typedef void (*AssertLogger)(const char* function,
                             const char* file,
                             int line,
                             const char* expression);

// Sets a custom assert logger to be used instead of the default LogAssert
// behavior. To clear the custom assert logger, pass NULL for |logger| and the
// default behavior will be restored. Only one custom assert logger can be set
// at a time, so this should generally be set during application startup and
// only by one component.
void SetCustomAssertLogger(AssertLogger logger);

}  // namespace rtc

#if ENABLE_DEBUG

namespace rtc {

inline bool Assert(bool result, const char* function, const char* file,
                   int line, const char* expression) {
  if (!result) {
    LogAssert(function, file, line, expression);
    Break();
    return false;
  }
  return true;
}

}  // namespace rtc

#if defined(_MSC_VER) && _MSC_VER < 1300
#define __FUNCTION__ ""
#endif

#ifndef ASSERT
#define ASSERT(x) \
    (void)rtc::Assert((x), __FUNCTION__, __FILE__, __LINE__, #x)
#endif

#ifndef VERIFY
#define VERIFY(x) rtc::Assert((x), __FUNCTION__, __FILE__, __LINE__, #x)
#endif

#else  // !ENABLE_DEBUG

namespace rtc {

inline bool ImplicitCastToBool(bool result) { return result; }

}  // namespace rtc

#ifndef ASSERT
#define ASSERT(x) (void)0
#endif

#ifndef VERIFY
#define VERIFY(x) rtc::ImplicitCastToBool(x)
#endif

#endif  // !ENABLE_DEBUG

#define COMPILE_TIME_ASSERT(expr)       char CTA_UNIQUE_NAME[expr]
#define CTA_UNIQUE_NAME                 CTA_MAKE_NAME(__LINE__)
#define CTA_MAKE_NAME(line)             CTA_MAKE_NAME2(line)
#define CTA_MAKE_NAME2(line)            constraint_ ## line

// Forces compiler to inline, even against its better judgement. Use wisely.
#if defined(__GNUC__)
#define FORCE_INLINE __attribute__((always_inline))
#elif defined(WEBRTC_WIN)
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE
#endif

// Borrowed from Chromium's base/compiler_specific.h.
// Annotate a virtual method indicating it must be overriding a virtual
// method in the parent class.
// Use like:
//   virtual void foo() OVERRIDE;
#if defined(WEBRTC_WIN)
#define OVERRIDE override
#elif defined(__clang__)
// Clang defaults to C++03 and warns about using override. Squelch that.
// Intentionally no push/pop here so all users of OVERRIDE ignore the warning
// too. This is like passing -Wno-c++11-extensions, except that GCC won't die
// (because it won't see this pragma).
#pragma clang diagnostic ignored "-Wc++11-extensions"
#define OVERRIDE override
#elif defined(__GNUC__) && __cplusplus >= 201103 && \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40700
// GCC 4.7 supports explicit virtual overrides when C++11 support is enabled.
#define OVERRIDE override
#else
#define OVERRIDE
#endif

// Annotate a function indicating the caller must examine the return value.
// Use like:
//   int foo() WARN_UNUSED_RESULT;
// To explicitly ignore a result, see |ignore_result()| in <base/basictypes.h>.
// TODO(ajm): Hack to avoid multiple definitions until the base/ of webrtc and
// libjingle are merged.
#if !defined(WARN_UNUSED_RESULT)
#if defined(__GNUC__)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif
#endif  // WARN_UNUSED_RESULT

#endif  // WEBRTC_BASE_COMMON_H_    // NOLINT
