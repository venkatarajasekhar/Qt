// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/animation/TimingInput.h"

#include "bindings/v8/Dictionary.h"
#include "core/css/parser/BisonCSSParser.h"
#include "core/css/resolver/CSSToStyleMap.h"

namespace WebCore {

void TimingInput::setStartDelay(Timing& timing, double startDelay)
{
    if (std::isfinite(startDelay))
        timing.startDelay = startDelay / 1000;
    else
        timing.startDelay = Timing::defaults().startDelay;
}

void TimingInput::setEndDelay(Timing& timing, double endDelay)
{
    if (std::isfinite(endDelay))
        timing.endDelay = endDelay / 1000;
    else
        timing.endDelay = Timing::defaults().endDelay;
}

void TimingInput::setFillMode(Timing& timing, const String& fillMode)
{
    if (fillMode == "none") {
        timing.fillMode = Timing::FillModeNone;
    } else if (fillMode == "backwards") {
        timing.fillMode = Timing::FillModeBackwards;
    } else if (fillMode == "both") {
        timing.fillMode = Timing::FillModeBoth;
    } else if (fillMode == "forwards") {
        timing.fillMode = Timing::FillModeForwards;
    } else {
        timing.fillMode = Timing::defaults().fillMode;
    }
}

void TimingInput::setIterationStart(Timing& timing, double iterationStart)
{
    if (std::isfinite(iterationStart))
        timing.iterationStart = std::max<double>(iterationStart, 0);
    else
        timing.iterationStart = Timing::defaults().iterationStart;
}

void TimingInput::setIterationCount(Timing& timing, double iterationCount)
{
    if (!std::isnan(iterationCount))
        timing.iterationCount = std::max<double>(iterationCount, 0);
    else
        timing.iterationCount = Timing::defaults().iterationCount;
}

void TimingInput::setIterationDuration(Timing& timing, double iterationDuration)
{
    if (!std::isnan(iterationDuration) && iterationDuration >= 0)
        timing.iterationDuration = iterationDuration / 1000;
    else
        timing.iterationDuration = Timing::defaults().iterationDuration;
}

void TimingInput::setPlaybackRate(Timing& timing, double playbackRate)
{
    if (std::isfinite(playbackRate))
        timing.playbackRate = playbackRate;
    else
        timing.playbackRate = Timing::defaults().playbackRate;
}

void TimingInput::setPlaybackDirection(Timing& timing, const String& direction)
{
    if (direction == "reverse") {
        timing.direction = Timing::PlaybackDirectionReverse;
    } else if (direction == "alternate") {
        timing.direction = Timing::PlaybackDirectionAlternate;
    } else if (direction == "alternate-reverse") {
        timing.direction = Timing::PlaybackDirectionAlternateReverse;
    } else {
        timing.direction = Timing::defaults().direction;
    }
}

void TimingInput::setTimingFunction(Timing& timing, const String& timingFunctionString)
{
    if (RefPtrWillBeRawPtr<CSSValue> timingFunctionValue = BisonCSSParser::parseAnimationTimingFunctionValue(timingFunctionString))
        timing.timingFunction = CSSToStyleMap::mapAnimationTimingFunction(timingFunctionValue.get(), true);
    else
        timing.timingFunction = Timing::defaults().timingFunction;
}

Timing TimingInput::convert(const Dictionary& timingInputDictionary)
{
    Timing result;

    // FIXME: This method needs to be refactored to handle invalid
    // null, NaN, Infinity values better.
    // See: http://www.w3.org/TR/WebIDL/#es-double
    double startDelay = Timing::defaults().startDelay;
    timingInputDictionary.get("delay", startDelay);
    setStartDelay(result, startDelay);

    double endDelay = Timing::defaults().endDelay;
    timingInputDictionary.get("endDelay", endDelay);
    setEndDelay(result, endDelay);

    String fillMode;
    timingInputDictionary.get("fill", fillMode);
    setFillMode(result, fillMode);

    double iterationStart = Timing::defaults().iterationStart;
    timingInputDictionary.get("iterationStart", iterationStart);
    setIterationStart(result, iterationStart);

    double iterationCount = Timing::defaults().iterationCount;
    timingInputDictionary.get("iterations", iterationCount);
    setIterationCount(result, iterationCount);

    double iterationDuration = 0;
    if (timingInputDictionary.get("duration", iterationDuration)) {
        setIterationDuration(result, iterationDuration);
    }

    double playbackRate = Timing::defaults().playbackRate;
    timingInputDictionary.get("playbackRate", playbackRate);
    setPlaybackRate(result, playbackRate);

    String direction;
    timingInputDictionary.get("direction", direction);
    setPlaybackDirection(result, direction);

    String timingFunctionString;
    timingInputDictionary.get("easing", timingFunctionString);
    setTimingFunction(result, timingFunctionString);

    result.assertValid();

    return result;
}

Timing TimingInput::convert(double duration)
{
    Timing result;
    setIterationDuration(result, duration);
    return result;
}

} // namespace WebCore
