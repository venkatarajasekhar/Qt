/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"
#include "SkColor.h"
#include "SkMatrixImageFilter.h"

namespace skiagm {

class MatrixImageFilterGM : public GM {
public:
    MatrixImageFilterGM() {
        this->setBGColor(0x00000000);
    }

protected:
    virtual SkString onShortName() {
        return SkString("matriximagefilter");
    }

    void draw(SkCanvas* canvas, const SkRect& rect, const SkBitmap& bitmap,
              const SkMatrix& matrix, SkPaint::FilterLevel filterLevel) {
        SkAutoTUnref<SkImageFilter> imageFilter(
            SkMatrixImageFilter::Create(matrix, filterLevel));
        SkPaint paint;
        paint.setImageFilter(imageFilter.get());
        canvas->saveLayer(&rect, &paint);
        canvas->drawBitmap(bitmap, 0, 0);
        canvas->restore();
    }

    virtual SkISize onISize() {
        return SkISize::Make(420, 100);
    }

    void make_checkerboard(SkBitmap* bitmap) {
        bitmap->allocN32Pixels(64, 64);
        SkCanvas canvas(*bitmap);
        SkPaint darkPaint;
        darkPaint.setColor(0xFF404040);
        SkPaint lightPaint;
        lightPaint.setColor(0xFFA0A0A0);
        for (int y = 0; y < 64; y += 32) {
            for (int x = 0; x < 64; x += 32) {
                canvas.save();
                canvas.translate(SkIntToScalar(x), SkIntToScalar(y));
                canvas.drawRect(SkRect::MakeXYWH(0, 0, 16, 16), darkPaint);
                canvas.drawRect(SkRect::MakeXYWH(16, 0, 16, 16), lightPaint);
                canvas.drawRect(SkRect::MakeXYWH(0, 16, 16, 16), lightPaint);
                canvas.drawRect(SkRect::MakeXYWH(16, 16, 16, 16), darkPaint);
                canvas.restore();
            }
        }
    }

    virtual void onDraw(SkCanvas* canvas) {
        canvas->clear(0x00000000);
        SkMatrix matrix;
        SkScalar margin = SkIntToScalar(10);
        matrix.setSkew(SkDoubleToScalar(0.5), SkDoubleToScalar(0.2));
        SkBitmap checkerboard;
        make_checkerboard(&checkerboard);

        SkRect srcRect = SkRect::MakeWH(96, 96);

        canvas->translate(margin, margin);
        draw(canvas, srcRect, checkerboard, matrix, SkPaint::kNone_FilterLevel);

        canvas->translate(srcRect.width() + margin, 0);
        draw(canvas, srcRect, checkerboard, matrix, SkPaint::kLow_FilterLevel);

#if 0
        // This may be causing Mac 10.6 to barf.
        canvas->translate(srcRect.width() + margin, 0);
        draw(canvas, srcRect, checkerboard, matrix, SkPaint::kMedium_FilterLevel);

        canvas->translate(srcRect.width() + margin, 0);
        draw(canvas, srcRect, checkerboard, matrix, SkPaint::kHigh_FilterLevel);
#endif
    }

private:
    typedef GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static GM* MyFactory(void*) { return new MatrixImageFilterGM; }
static GMRegistry reg(MyFactory);

}
