
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */



#include "gm.h"
#include "SkRandom.h"

namespace skiagm {

#define W   400
#define H   400
#define N   100

static const SkScalar SW = SkIntToScalar(W);
static const SkScalar SH = SkIntToScalar(H);

class StrokeRectsGM : public GM {
public:
    StrokeRectsGM() {}

protected:
    virtual uint32_t onGetFlags() const SK_OVERRIDE {
        return kSkipTiled_Flag;
    }

    virtual SkString onShortName() {
        return SkString("strokerects");
    }

    virtual SkISize onISize() {
        return SkISize::Make(W*2, H*2);
    }

    static void rnd_rect(SkRect* r, SkLCGRandom& rand) {
        SkScalar x = rand.nextUScalar1() * W;
        SkScalar y = rand.nextUScalar1() * H;
        SkScalar w = rand.nextUScalar1() * (W >> 2);
        SkScalar h = rand.nextUScalar1() * (H >> 2);
        SkScalar hoffset = rand.nextSScalar1();
        SkScalar woffset = rand.nextSScalar1();

        r->set(x, y, x + w, y + h);
        r->offset(-w/2 + woffset, -h/2 + hoffset);
    }

    virtual void onDraw(SkCanvas* canvas) {
        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);

        for (int y = 0; y < 2; y++) {
            paint.setAntiAlias(!!y);
            for (int x = 0; x < 2; x++) {
                paint.setStrokeWidth(x * SkIntToScalar(3));

                SkAutoCanvasRestore acr(canvas, true);
                canvas->translate(SW * x, SH * y);
                canvas->clipRect(SkRect::MakeLTRB(
                        SkIntToScalar(2), SkIntToScalar(2)
                        , SW - SkIntToScalar(2), SH - SkIntToScalar(2)
                ));

                SkLCGRandom rand;
                for (int i = 0; i < N; i++) {
                    SkRect r;
                    rnd_rect(&r, rand);
                    canvas->drawRect(r, paint);
                }
            }
        }
    }

private:
    typedef GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static GM* MyFactory(void*) { return new StrokeRectsGM; }
static GMRegistry reg(MyFactory);

}
