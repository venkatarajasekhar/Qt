
/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "gm.h"

#include "SkColorPriv.h"
#include "SkShader.h"

namespace skiagm {

class BigMatrixGM : public GM {
public:
    BigMatrixGM() {
        this->setBGColor(0xFF66AA99);
    }

protected:
    virtual uint32_t onGetFlags() const SK_OVERRIDE {
        return kSkipTiled_Flag;
    }

    virtual SkString onShortName() {
        return SkString("bigmatrix");
    }

    virtual SkISize onISize() {
        return SkISize::Make(50, 50);
    }

    virtual void onDraw(SkCanvas* canvas) {
        SkMatrix m;
        m.reset();
        m.setRotate(33 * SK_Scalar1);
        m.postScale(3000 * SK_Scalar1, 3000 * SK_Scalar1);
        m.postTranslate(6000 * SK_Scalar1, -5000 * SK_Scalar1);
        canvas->concat(m);

        SkPaint paint;
        paint.setColor(SK_ColorRED);
        paint.setAntiAlias(true);

        bool success = m.invert(&m);
        SkASSERT(success);
        (void) success; // silence compiler :(

        SkPath path;

        SkPoint pt = {10 * SK_Scalar1, 10 * SK_Scalar1};
        SkScalar small = 1 / (500 * SK_Scalar1);

        m.mapPoints(&pt, 1);
        path.addCircle(pt.fX, pt.fY, small);
        canvas->drawPath(path, paint);

        pt.set(30 * SK_Scalar1, 10 * SK_Scalar1);
        m.mapPoints(&pt, 1);
        SkRect rect = {pt.fX - small, pt.fY - small,
                       pt.fX + small, pt.fY + small};
        canvas->drawRect(rect, paint);

        SkBitmap bmp;
        bmp.allocN32Pixels(2, 2);
        uint32_t* pixels = reinterpret_cast<uint32_t*>(bmp.getPixels());
        pixels[0] = SkPackARGB32(0xFF, 0xFF, 0x00, 0x00);
        pixels[1] = SkPackARGB32(0xFF, 0x00, 0xFF, 0x00);
        pixels[2] = SkPackARGB32(0x80, 0x00, 0x00, 0x00);
        pixels[3] = SkPackARGB32(0xFF, 0x00, 0x00, 0xFF);
        pt.set(30 * SK_Scalar1, 30 * SK_Scalar1);
        m.mapPoints(&pt, 1);
        SkMatrix s;
        s.reset();
        s.setScale(SK_Scalar1 / 1000, SK_Scalar1 / 1000);
        SkShader* shader = SkShader::CreateBitmapShader(
                                            bmp,
                                            SkShader::kRepeat_TileMode,
                                            SkShader::kRepeat_TileMode,
                                            &s);
        paint.setShader(shader)->unref();
        paint.setAntiAlias(false);
        paint.setFilterLevel(SkPaint::kLow_FilterLevel);
        rect.setLTRB(pt.fX - small, pt.fY - small,
                     pt.fX + small, pt.fY + small);
        canvas->drawRect(rect, paint);
    }

private:
    typedef GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static GM* MyFactory(void*) { return new BigMatrixGM; }
static GMRegistry reg(MyFactory);

}
