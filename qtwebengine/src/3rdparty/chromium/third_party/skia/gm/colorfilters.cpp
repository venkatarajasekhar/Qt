/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"
#include "SkCanvas.h"
#include "SkColorFilter.h"
#include "SkGradientShader.h"

static SkShader* make_shader(const SkRect& bounds) {
    const SkPoint pts[] = {
        { bounds.left(), bounds.top() },
        { bounds.right(), bounds.bottom() },
    };
    const SkColor colors[] = {
        SK_ColorRED, SK_ColorGREEN, SK_ColorBLUE, SK_ColorBLACK,
        SK_ColorCYAN, SK_ColorMAGENTA, SK_ColorYELLOW,
    };
    return SkGradientShader::CreateLinear(pts,
                                          colors, NULL, SK_ARRAY_COUNT(colors),
                                          SkShader::kClamp_TileMode);
}

typedef void (*InstallPaint)(SkPaint*, uint32_t, uint32_t);

static void install_nothing(SkPaint* paint, uint32_t, uint32_t) {
    paint->setColorFilter(NULL);
}

static void install_lighting(SkPaint* paint, uint32_t mul, uint32_t add) {
    paint->setColorFilter(SkColorFilter::CreateLightingFilter(mul, add))->unref();
}

class ColorFiltersGM : public skiagm::GM {
public:
    ColorFiltersGM() {
        fName.set("lightingcolorfilter");
    }

protected:
    virtual SkString onShortName() {
        return fName;
    }

    virtual SkISize onISize() {
        return SkISize::Make(640, 480);
    }

    virtual uint32_t onGetFlags() const SK_OVERRIDE {
        return kSkipTiled_Flag;
    }

    virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE {
        SkPaint paint;
        SkRect r;
        r.setWH(600, 50);
        paint.setShader(make_shader(r))->unref();

        const struct {
            InstallPaint    fProc;
            uint32_t        fData0, fData1;
        } rec[] = {
            { install_nothing, 0, 0 },
            { install_lighting, 0xFF0000, 0 },
            { install_lighting, 0x00FF00, 0 },
            { install_lighting, 0x0000FF, 0 },
            { install_lighting, 0x000000, 0xFF0000 },
            { install_lighting, 0x000000, 0x00FF00 },
            { install_lighting, 0x000000, 0x0000FF },
        };

        canvas->translate(10, 10);
        for (size_t i = 0; i < SK_ARRAY_COUNT(rec); ++i) {
            rec[i].fProc(&paint, rec[i].fData0, rec[i].fData1);
            canvas->drawRect(r, paint);
            canvas->translate(0, r.height() + 10);
        }
    }

private:
    SkString fName;
    typedef GM INHERITED;
};


//////////////////////////////////////////////////////////////////////////////

DEF_GM( return SkNEW(ColorFiltersGM); )
