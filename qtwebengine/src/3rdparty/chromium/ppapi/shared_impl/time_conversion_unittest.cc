// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <math.h>
#include <stdlib.h>

#include "ppapi/shared_impl/time_conversion.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ppapi {

// Slop we'll allow in two Time "internal values" to consider them equal.
// Double conversion can introduce rounding errors. The internal values are in
// microseconds, so an error here is very small.
static const int kTimeInternalValueSlop = 2;

// Same as above in double-precision seconds units.
static const double kTimeSecondsSlop =
    static_cast<double>(kTimeInternalValueSlop) /
    base::Time::kMicrosecondsPerSecond;

TEST(TimeConversion, Time) {
  // Should be able to round-trip.
  base::Time now = base::Time::Now();
  base::Time converted = ppapi::PPTimeToTime(TimeToPPTime(now));
  EXPECT_GE(kTimeInternalValueSlop,
            abs(static_cast<int>((converted - now).ToInternalValue())));

  // Units should be in seconds.
  base::Time one_second_from_now = now + base::TimeDelta::FromSeconds(1);
  double converted_one_second_from_now =
      ppapi::TimeToPPTime(one_second_from_now) - ppapi::TimeToPPTime(now);
  EXPECT_GE(kTimeSecondsSlop, fabs(converted_one_second_from_now - 1));
}

TEST(TimeConversion, EventTime) {
  // Should be able to round-trip.
  base::Time now = base::Time::Now();
  double event_now = now.ToDoubleT();
  double converted =
      ppapi::EventTimeToPPTimeTicks(ppapi::PPTimeTicksToEventTime(event_now));
  EXPECT_GE(kTimeSecondsSlop, fabs(converted - event_now));

  // Units should be in seconds.
  base::Time one_second_from_now = now + base::TimeDelta::FromSeconds(1);
  double event_one_second_from_now = one_second_from_now.ToDoubleT();
  EXPECT_GE(kTimeSecondsSlop,
            1.0 - ppapi::EventTimeToPPTimeTicks(event_one_second_from_now) -
                ppapi::EventTimeToPPTimeTicks(event_now));
}

TEST(TimeConversion, EpochTime) {
  // Should be able to round-trip from epoch time.
  base::Time epoch = base::Time::UnixEpoch();
  base::Time converted = ppapi::PPTimeToTime(TimeToPPTime(epoch));
  EXPECT_GE(kTimeInternalValueSlop,
            abs(static_cast<int>((converted - epoch).ToInternalValue())));

  // Units should be in seconds.
  base::Time one_second_from_epoch = epoch + base::TimeDelta::FromSeconds(1);
  double converted_one_second_from_epoch =
      ppapi::TimeToPPTime(one_second_from_epoch) - ppapi::TimeToPPTime(epoch);
  EXPECT_GE(kTimeSecondsSlop, fabs(converted_one_second_from_epoch - 1));

  // Epoch time should be equal to a PP_Time of 0.0.
  EXPECT_GE(kTimeSecondsSlop, fabs(ppapi::TimeToPPTime(epoch) - 0.0));
  EXPECT_GE(kTimeInternalValueSlop,
            abs(static_cast<int>(
                (ppapi::PPTimeToTime(0.0) - epoch).ToInternalValue())));
}

}  // namespace ppapi
