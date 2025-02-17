/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"
#include "SkCanvas.h"
#include "SkRRect.h"
#include "SkPath.h"

class DRRectGM : public skiagm::GM {
public:
    DRRectGM() {}

protected:
    virtual uint32_t onGetFlags() const SK_OVERRIDE {
        return kSkipTiled_Flag;
    }

    virtual SkString onShortName() SK_OVERRIDE {
        return SkString("drrect");
    }

    virtual SkISize onISize() SK_OVERRIDE {
        return SkISize::Make(640, 480);
    }

    virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE {
        SkPaint paint;
        paint.setAntiAlias(true);

        SkRRect outers[4];
        // like squares/circles, to exercise fast-cases in GPU
        SkRect r = { 0, 0, 100, 100 };
        SkVector radii[4] = {
            { 0, 0 }, { 30, 1 }, { 10, 40 }, { 40, 40 }
        };

        const SkScalar dx = r.width() + 16;
        const SkScalar dy = r.height() + 16;

        outers[0].setRect(r);
        outers[1].setOval(r);
        outers[2].setRectXY(r, 20, 20);
        outers[3].setRectRadii(r, radii);

        SkRRect inners[5];
        r.inset(25, 25);

        inners[0].setEmpty();
        inners[1].setRect(r);
        inners[2].setOval(r);
        inners[3].setRectXY(r, 20, 20);
        inners[4].setRectRadii(r, radii);

        canvas->translate(16, 16);
        for (size_t j = 0; j < SK_ARRAY_COUNT(inners); ++j) {
            for (size_t i = 0; i < SK_ARRAY_COUNT(outers); ++i) {
                canvas->save();
                canvas->translate(dx * j, dy * i);
                canvas->drawDRRect(outers[i], inners[j], paint);
                canvas->restore();
            }
        }
    }

private:
    typedef GM INHERITED;
};

DEF_GM( return new DRRectGM; )
