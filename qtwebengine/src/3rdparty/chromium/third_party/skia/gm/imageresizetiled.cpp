/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"
#include "SkMatrixImageFilter.h"
#include "SkRandom.h"

#define WIDTH 640
#define HEIGHT 480

#define RESIZE_FACTOR SkIntToScalar(2)

namespace skiagm {

class ImageResizeTiledGM : public GM {
public:
    ImageResizeTiledGM() {
    }

protected:
    virtual SkString onShortName() SK_OVERRIDE {
        return SkString("imageresizetiled");
    }

    virtual SkISize onISize() SK_OVERRIDE {
        return SkISize::Make(WIDTH, HEIGHT);
    }

    virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE {
        SkPaint paint;
        SkMatrix matrix;
        matrix.setScale(RESIZE_FACTOR, RESIZE_FACTOR);
        SkAutoTUnref<SkImageFilter> imageFilter(
            SkMatrixImageFilter::Create(matrix, SkPaint::kNone_FilterLevel));
        paint.setImageFilter(imageFilter.get());
        const SkScalar tile_size = SkIntToScalar(100);
        SkRect bounds;
        canvas->getClipBounds(&bounds);
        for (SkScalar y = 0; y < HEIGHT; y += tile_size) {
            for (SkScalar x = 0; x < WIDTH; x += tile_size) {
                canvas->save();
                canvas->clipRect(SkRect::MakeXYWH(x, y, tile_size, tile_size));
                canvas->scale(SkScalarInvert(RESIZE_FACTOR),
                              SkScalarInvert(RESIZE_FACTOR));
                canvas->saveLayer(NULL, &paint);
                const char* str[] = {
                    "The quick",
                    "brown fox",
                    "jumped over",
                    "the lazy dog.",
                };
                SkPaint textPaint;
                textPaint.setAntiAlias(true);
                textPaint.setTextSize(SkIntToScalar(100));
                int posY = 0;
                for (unsigned i = 0; i < SK_ARRAY_COUNT(str); i++) {
                    posY += 100;
                    canvas->drawText(str[i], strlen(str[i]), SkIntToScalar(0),
                                     SkIntToScalar(posY), textPaint);
                }
                canvas->restore();
                canvas->restore();
            }
        }
    }

private:
    typedef GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

DEF_GM(return new ImageResizeTiledGM(); )

}
