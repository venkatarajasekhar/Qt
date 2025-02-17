
/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Benchmark.h"
#include "SkCanvas.h"
#include "SkPaint.h"
#include "SkRandom.h"

/**
 * Draws full screen opaque rectangles. It is designed to test any optimizations in the GPU backend
 * to turn such draws into clears.
 */
class FSRectBench : public Benchmark {
public:
    FSRectBench() : fInit(false) { }

protected:
    virtual const char* onGetName() SK_OVERRIDE { return "fullscreen_rects"; }

    virtual void onPreDraw() SK_OVERRIDE {
        if (!fInit) {
            SkRandom rand;
            static const SkScalar kMinOffset = 0;
            static const SkScalar kMaxOffset = 100 * SK_Scalar1;
            static const SkScalar kOffsetRange = kMaxOffset - kMinOffset;
            for (int i = 0; i < N; ++i) {
                fRects[i].fLeft = -kMinOffset - SkScalarMul(rand.nextUScalar1(), kOffsetRange);
                fRects[i].fTop = -kMinOffset - SkScalarMul(rand.nextUScalar1(), kOffsetRange);
                fRects[i].fRight = W + kMinOffset + SkScalarMul(rand.nextUScalar1(), kOffsetRange);
                fRects[i].fBottom = H + kMinOffset + SkScalarMul(rand.nextUScalar1(), kOffsetRange);
                fColors[i] = rand.nextU() | 0xFF000000;
            }
            fInit = true;
        }
    }

    virtual void onDraw(const int loops, SkCanvas* canvas) SK_OVERRIDE {
        SkPaint paint;
        for (int i = 0; i < loops; ++i) {
            paint.setColor(fColors[i % N]);
            canvas->drawRect(fRects[i % N], paint);
        }
    }

private:
    enum {
        W = 640,
        H = 480,
        N = 300,
    };
    SkRect  fRects[N];
    SkColor fColors[N];
    bool fInit;

    typedef Benchmark INHERITED;
};

DEF_BENCH( return SkNEW_ARGS(FSRectBench, ()); )
