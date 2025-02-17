/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"
#include "SkColorFilter.h"
#include "SkBlurMaskFilter.h"

namespace skiagm {

/**
 * This test exercises bug 1719. An anti-aliased blurred path is rendered through a soft clip. On
 * the GPU a scratch texture was used to hold the original path mask as well as the blurred path
 * result. The same texture is then incorrectly used to generate the soft clip mask for the draw.
 * Thus the same texture is used for both the blur mask and soft mask in a single draw.
 *
 * The correct image should look like a thin stroked round rect.
 */
class SkBug1719GM : public GM {
public:
    SkBug1719GM() {}

protected:
    virtual uint32_t onGetFlags() const SK_OVERRIDE {
        return kSkipTiled_Flag;
    }

    virtual SkString onShortName() SK_OVERRIDE {
        return SkString("skbug1719");
    }

    virtual SkISize onISize() SK_OVERRIDE {
        return SkISize::Make(300, 100);
    }

    virtual void onDrawBackground(SkCanvas* canvas) SK_OVERRIDE {
        SkPaint bgPaint;
        bgPaint.setColor(0xFF303030);
        canvas->drawPaint(bgPaint);
    }

    virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE {
        canvas->translate(SkIntToScalar(-800), SkIntToScalar(-650));

        // The data is lifted from an SKP that exhibited the bug.

        // This is a round rect.
        SkPath clipPath;
        clipPath.moveTo(832.f, 654.f);
        clipPath.lineTo(1034.f, 654.f);
        clipPath.cubicTo(1038.4183f, 654.f, 1042.f, 657.58173f, 1042.f, 662.f);
        clipPath.lineTo(1042.f, 724.f);
        clipPath.cubicTo(1042.f, 728.41827f, 1038.4183f, 732.f, 1034.f, 732.f);
        clipPath.lineTo(832.f, 732.f);
        clipPath.cubicTo(827.58173f, 732.f, 824.f, 728.41827f, 824.f, 724.f);
        clipPath.lineTo(824.f, 662.f);
        clipPath.cubicTo(824.f, 657.58173f, 827.58173f, 654.f, 832.f, 654.f);
        clipPath.close();

        // This is a round rect nested inside a rect.
        SkPath drawPath;
        drawPath.moveTo(823.f, 653.f);
        drawPath.lineTo(1043.f, 653.f);
        drawPath.lineTo(1043.f, 733.f);
        drawPath.lineTo(823.f, 733.f);
        drawPath.lineTo(823.f, 653.f);
        drawPath.close();
        drawPath.moveTo(832.f, 654.f);
        drawPath.lineTo(1034.f, 654.f);
        drawPath.cubicTo(1038.4183f, 654.f, 1042.f, 657.58173f, 1042.f, 662.f);
        drawPath.lineTo(1042.f, 724.f);
        drawPath.cubicTo(1042.f, 728.41827f, 1038.4183f, 732.f, 1034.f, 732.f);
        drawPath.lineTo(832.f, 732.f);
        drawPath.cubicTo(827.58173f, 732.f, 824.f, 728.41827f, 824.f, 724.f);
        drawPath.lineTo(824.f, 662.f);
        drawPath.cubicTo(824.f, 657.58173f, 827.58173f, 654.f, 832.f, 654.f);
        drawPath.close();
        drawPath.setFillType(SkPath::kEvenOdd_FillType);

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(0xFF000000);
        paint.setMaskFilter(
            SkBlurMaskFilter::Create(kNormal_SkBlurStyle,
                                     0.78867501f,
                                     SkBlurMaskFilter::kHighQuality_BlurFlag))->unref();
        paint.setColorFilter(
            SkColorFilter::CreateModeFilter(0xBFFFFFFF, SkXfermode::kSrcIn_Mode))->unref();

        canvas->clipPath(clipPath, SkRegion::kIntersect_Op, true);
        canvas->drawPath(drawPath, paint);
    }

private:

    typedef GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

DEF_GM(return new SkBug1719GM;)

}
