/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"
#include "SkCanvas.h"
#include "SkGradientShader.h"

class AlphaGradientsGM : public skiagm::GM {
public:
    AlphaGradientsGM() {}

protected:
    virtual SkString onShortName() SK_OVERRIDE {
        return SkString("alphagradients");
    }

    virtual SkISize onISize() SK_OVERRIDE {
        return SkISize::Make(640, 480);
    }

    static void draw_grad(SkCanvas* canvas, const SkRect& r,
                          SkColor c0, SkColor c1, bool doPreMul) {
        SkColor colors[] = { c0, c1 };
        SkPoint pts[] = { { r.fLeft, r.fTop }, { r.fRight, r.fBottom } };
        SkPaint paint;
        uint32_t flags = doPreMul ? SkGradientShader::kInterpolateColorsInPremul_Flag : 0;
        SkShader* s = SkGradientShader::CreateLinear(pts, colors, NULL, 2,
                                                     SkShader::kClamp_TileMode, flags, NULL);
        paint.setShader(s)->unref();
        canvas->drawRect(r, paint);

        paint.setShader(NULL);
        paint.setStyle(SkPaint::kStroke_Style);
        canvas->drawRect(r, paint);
    }

    virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE {
        static const struct {
            SkColor fColor0;
            SkColor fColor1;
        } gRec[] = {
            { 0xFFFFFFFF, 0x00000000 },
            { 0xFFFFFFFF, 0x00FF0000 },
            { 0xFFFFFFFF, 0x00FFFF00 },
            { 0xFFFFFFFF, 0x00FFFFFF },
            { 0xFFFF0000, 0x00000000 },
            { 0xFFFF0000, 0x00FF0000 },
            { 0xFFFF0000, 0x00FFFF00 },
            { 0xFFFF0000, 0x00FFFFFF },
            { 0xFF0000FF, 0x00000000 },
            { 0xFF0000FF, 0x00FF0000 },
            { 0xFF0000FF, 0x00FFFF00 },
            { 0xFF0000FF, 0x00FFFFFF },
        };

        SkRect r = SkRect::MakeWH(300, 30);

        canvas->translate(10, 10);

        for (int doPreMul = 0; doPreMul <= 1; ++doPreMul) {
            canvas->save();
            for (size_t i = 0; i < SK_ARRAY_COUNT(gRec); ++i) {
                draw_grad(canvas, r, gRec[i].fColor0, gRec[i].fColor1, SkToBool(doPreMul));
                canvas->translate(0, r.height() + 8);
            }
            canvas->restore();
            canvas->translate(r.width() + 10, 0);
        }
    }

    virtual uint32_t onGetFlags() const { return kSkipPipe_Flag | kSkipTiled_Flag; }

private:
    typedef skiagm::GM INHERITED;
};

DEF_GM( return SkNEW(AlphaGradientsGM); )
