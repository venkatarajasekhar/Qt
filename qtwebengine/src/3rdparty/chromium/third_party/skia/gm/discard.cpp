/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#if SK_SUPPORT_GPU

#include "gm.h"
#include "SkCanvas.h"
#include "SkColorShader.h"
#include "SkPaint.h"
#include "SkSurface.h"

namespace skiagm {

/*
 * This GM exercises SkCanvas::discard() by creating an offscreen SkSurface and repeatedly
 * discarding it, drawing to it, and then drawing it to the main canvas.
 */
class DiscardGM : public GM {

public:
    DiscardGM() {
    }

    virtual uint32_t onGetFlags() const SK_OVERRIDE { return kGPUOnly_Flag; }

protected:
    virtual SkString onShortName() SK_OVERRIDE {
        return SkString("discard");
    }

    virtual SkISize onISize() SK_OVERRIDE {
        return SkISize::Make(100, 100);
    }

    virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE {
        GrContext* context = canvas->getGrContext();
        if (NULL == context) {
            return;
        }

        SkISize size = this->getISize();
        size.fWidth /= 10;
        size.fHeight /= 10;
        SkImageInfo info = SkImageInfo::MakeN32Premul(size);
        SkSurface* surface = SkSurface::NewRenderTarget(context, info);

        if (NULL == surface) {
            return;
        }

        canvas->clear(SK_ColorBLACK);

        SkRandom rand;
        for (int x = 0; x < 10; ++x) {
            for (int y = 0; y < 10; ++y) {
              surface->getCanvas()->discard();
              // Make something that isn't too close to the background color, black.
              SkColor color = rand.nextU() | 0xFF404040;
              switch (rand.nextULessThan(3)) {
                  case 0:
                      surface->getCanvas()->drawColor(color);
                      break;
                  case 1:
                      surface->getCanvas()->clear(color);
                      break;
                  case 2:
                      SkColorShader shader(color);
                      SkPaint paint;
                      paint.setShader(&shader);
                      surface->getCanvas()->drawPaint(paint);
                      break;
              }
              surface->draw(canvas, 10.f*x, 10.f*y, NULL);
            }
        }

        surface->getCanvas()->discard();
        surface->unref();
    }

private:
    typedef GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

DEF_GM( return SkNEW(DiscardGM); )

} // end namespace

#endif
