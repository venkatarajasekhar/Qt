/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"
#include "SkCanvas.h"
#include "SkColorPriv.h"
#include "SkShader.h"

static const struct {
    SkXfermode::Mode  fMode;
    const char*         fLabel;
} gModes[] = {
    { SkXfermode::kClear_Mode,    "Clear"     },
    { SkXfermode::kSrc_Mode,      "Src"       },
    { SkXfermode::kDst_Mode,      "Dst"       },
    { SkXfermode::kSrcOver_Mode,  "SrcOver"   },
    { SkXfermode::kDstOver_Mode,  "DstOver"   },
    { SkXfermode::kSrcIn_Mode,    "SrcIn"     },
    { SkXfermode::kDstIn_Mode,    "DstIn"     },
    { SkXfermode::kSrcOut_Mode,   "SrcOut"    },
    { SkXfermode::kDstOut_Mode,   "DstOut"    },
    { SkXfermode::kSrcATop_Mode,  "SrcATop"   },
    { SkXfermode::kDstATop_Mode,  "DstATop"   },
    { SkXfermode::kXor_Mode,      "Xor"       },
};

const int gWidth = 64;
const int gHeight = 64;
const SkScalar W = SkIntToScalar(gWidth);
const SkScalar H = SkIntToScalar(gHeight);

static SkScalar drawCell(SkCanvas* canvas, SkXfermode* mode, SkAlpha a0, SkAlpha a1) {

    SkPaint paint;
    paint.setAntiAlias(true);

    SkRect r = SkRect::MakeWH(W, H);
    r.inset(W/10, H/10);

    paint.setColor(SK_ColorBLUE);
    paint.setAlpha(a0);
    canvas->drawOval(r, paint);

    paint.setColor(SK_ColorRED);
    paint.setAlpha(a1);
    paint.setXfermode(mode);
    for (int angle = 0; angle < 24; ++angle) {
        SkScalar x = SkScalarCos(SkIntToScalar(angle) * (SK_ScalarPI * 2) / 24) * gWidth;
        SkScalar y = SkScalarSin(SkIntToScalar(angle) * (SK_ScalarPI * 2) / 24) * gHeight;
        paint.setStrokeWidth(SK_Scalar1 * angle * 2 / 24);
        canvas->drawLine(W/2, H/2, W/2 + x, H/2 + y, paint);
    }

    return H;
}

static SkShader* make_bg_shader() {
    SkBitmap bm;
    bm.allocN32Pixels(2, 2);
    *bm.getAddr32(0, 0) = *bm.getAddr32(1, 1) = 0xFFFFFFFF;
    *bm.getAddr32(1, 0) = *bm.getAddr32(0, 1) = SkPackARGB32(0xFF, 0xCC, 0xCC, 0xCC);

    SkMatrix m;
    m.setScale(SkIntToScalar(6), SkIntToScalar(6));
    return SkShader::CreateBitmapShader(bm,
                                        SkShader::kRepeat_TileMode,
                                        SkShader::kRepeat_TileMode,
                                        &m);
}

namespace skiagm {

    class HairModesGM : public GM {
        SkPaint fBGPaint;

    protected:
        virtual SkString onShortName() SK_OVERRIDE {
            return SkString("hairmodes");
        }

        virtual SkISize onISize() { return SkISize::Make(640, 480); }

        virtual void onOnceBeforeDraw() SK_OVERRIDE {
            fBGPaint.setShader(make_bg_shader())->unref();
        }

        virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE {
            const SkRect bounds = SkRect::MakeWH(W, H);
            static const SkAlpha gAlphaValue[] = { 0xFF, 0x88, 0x88 };

            canvas->translate(SkIntToScalar(4), SkIntToScalar(4));

            for (int alpha = 0; alpha < 4; ++alpha) {
                canvas->save();
                canvas->save();
                for (size_t i = 0; i < SK_ARRAY_COUNT(gModes); ++i) {
                    if (6 == i) {
                        canvas->restore();
                        canvas->translate(W * 5, 0);
                        canvas->save();
                    }
                    SkXfermode* mode = SkXfermode::Create(gModes[i].fMode);

                    canvas->drawRect(bounds, fBGPaint);
                    canvas->saveLayer(&bounds, NULL);
                    SkScalar dy = drawCell(canvas, mode,
                                           gAlphaValue[alpha & 1],
                                           gAlphaValue[alpha & 2]);
                    canvas->restore();

                    canvas->translate(0, dy * 5 / 4);
                    SkSafeUnref(mode);
                }
                canvas->restore();
                canvas->restore();
                canvas->translate(W * 5 / 4, 0);
            }
        }

        // disable pdf for now, since it crashes on mac
        virtual uint32_t onGetFlags() const { return kSkipPDF_Flag | kSkipTiled_Flag; }

    private:
        typedef GM INHERITED;
    };

    //////////////////////////////////////////////////////////////////////////////

    static GM* MyFactory(void*) { return new HairModesGM; }
    static GMRegistry reg(MyFactory);

}
