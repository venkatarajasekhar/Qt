
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "gm.h"
#include "SkCanvas.h"
#include "SkGradientShader.h"
#include "SkGraphics.h"
#include "SkPath.h"
#include "SkRegion.h"
#include "SkShader.h"

static void make_bitmap(SkBitmap* bitmap) {
    bitmap->allocN32Pixels(64, 64);

    SkCanvas canvas(*bitmap);

    canvas.drawColor(SK_ColorRED);
    SkPaint paint;
    paint.setAntiAlias(true);
    const SkPoint pts[] = { { 0, 0 }, { 64, 64 } };
    const SkColor colors[] = { SK_ColorWHITE, SK_ColorBLUE };
    paint.setShader(SkGradientShader::CreateLinear(pts, colors, NULL, 2,
                                                   SkShader::kClamp_TileMode))->unref();
    canvas.drawCircle(32, 32, 32, paint);
}

class DrawBitmapRect2 : public skiagm::GM {
    bool fUseIRect;
public:
    DrawBitmapRect2(bool useIRect) : fUseIRect(useIRect) {
    }

protected:
    virtual SkString onShortName() SK_OVERRIDE {
        SkString str;
        str.printf("bitmaprect_%s", fUseIRect ? "i" : "s");
        return str;
    }

    virtual SkISize onISize() SK_OVERRIDE {
        return SkISize::Make(640, 480);
    }

    virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE {
        canvas->drawColor(0xFFCCCCCC);

        const SkIRect src[] = {
            { 0, 0, 32, 32 },
            { 0, 0, 80, 80 },
            { 32, 32, 96, 96 },
            { -32, -32, 32, 32, }
        };

        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);

        SkBitmap bitmap;
        make_bitmap(&bitmap);

        SkRect dstR = { 0, 200, 128, 380 };

        canvas->translate(16, 40);
        for (size_t i = 0; i < SK_ARRAY_COUNT(src); i++) {
            SkRect srcR;
            srcR.set(src[i]);

            canvas->drawBitmap(bitmap, 0, 0, &paint);
            if (!fUseIRect) {
                canvas->drawBitmapRectToRect(bitmap, &srcR, dstR, &paint);
            } else {
                canvas->drawBitmapRect(bitmap, &src[i], dstR, &paint);
            }

            canvas->drawRect(dstR, paint);
            canvas->drawRect(srcR, paint);

            canvas->translate(160, 0);
        }
    }

private:
    typedef skiagm::GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////
static void make_3x3_bitmap(SkBitmap* bitmap) {

    static const int gXSize = 3;
    static const int gYSize = 3;

    SkColor textureData[gXSize][gYSize] = {
        { SK_ColorRED,    SK_ColorWHITE, SK_ColorBLUE },
        { SK_ColorGREEN,  SK_ColorBLACK, SK_ColorCYAN },
        { SK_ColorYELLOW, SK_ColorGRAY,  SK_ColorMAGENTA }
    };


    bitmap->allocN32Pixels(gXSize, gYSize);
    for (int y = 0; y < gYSize; y++) {
        for (int x = 0; x < gXSize; x++) {
            *bitmap->getAddr32(x, y) = textureData[x][y];
        }
    }
}

// This GM attempts to make visible any issues drawBitmapRectToRect may have
// with partial source rects. In this case the eight pixels on the border
// should be half the width/height of the central pixel, i.e.:
//                         __|____|__
//                           |    |
//                         __|____|__
//                           |    |
class DrawBitmapRect3 : public skiagm::GM {
public:
    DrawBitmapRect3() {
        this->setBGColor(SK_ColorBLACK);
    }

protected:
    virtual SkString onShortName() SK_OVERRIDE {
        SkString str;
        str.printf("3x3bitmaprect");
        return str;
    }

    virtual SkISize onISize() SK_OVERRIDE {
        return SkISize::Make(640, 480);
    }

    virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE {

        SkBitmap bitmap;
        make_3x3_bitmap(&bitmap);

        SkRect srcR = { 0.5f, 0.5f, 2.5f, 2.5f };
        SkRect dstR = { 100, 100, 300, 200 };

        canvas->drawBitmapRectToRect(bitmap, &srcR, dstR, NULL);
    }

private:
    typedef skiagm::GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////
static void make_big_bitmap(SkBitmap* bitmap) {

    static const int gXSize = 4096;
    static const int gYSize = 4096;
    static const int gBorderWidth = 10;

    bitmap->allocN32Pixels(gXSize, gYSize);
    for (int y = 0; y < gYSize; ++y) {
        for (int x = 0; x < gXSize; ++x) {
            if (x <= gBorderWidth || x >= gXSize-gBorderWidth ||
                y <= gBorderWidth || y >= gYSize-gBorderWidth) {
                *bitmap->getAddr32(x, y) = 0x88FFFFFF;
            } else {
                *bitmap->getAddr32(x, y) = 0x88FF0000;
            }
        }
    }
}

// This GM attempts to reveal any issues we may have when the GPU has to
// break up a large texture in order to draw it. The XOR transfer mode will
// create stripes in the image if there is imprecision in the destination
// tile placement.
class DrawBitmapRect4 : public skiagm::GM {
    bool fUseIRect;
    SkBitmap fBigBitmap;

public:
    DrawBitmapRect4(bool useIRect) : fUseIRect(useIRect) {
        this->setBGColor(0x88444444);
    }

protected:
    virtual SkString onShortName() SK_OVERRIDE {
        SkString str;
        str.printf("bigbitmaprect_%s", fUseIRect ? "i" : "s");
        return str;
    }

    virtual SkISize onISize() SK_OVERRIDE {
        return SkISize::Make(640, 480);
    }

    virtual void onOnceBeforeDraw() SK_OVERRIDE {
        make_big_bitmap(&fBigBitmap);
    }

    virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE {

        SkXfermode* mode = SkXfermode::Create(SkXfermode::kXor_Mode);

        SkPaint paint;
        paint.setAlpha(128);
        paint.setXfermode(mode)->unref();

        SkRect srcR1 = { 0.0f, 0.0f, 4096.0f, 2040.0f };
        SkRect dstR1 = { 10.1f, 10.1f, 629.9f, 400.9f };

        SkRect srcR2 = { 4085.0f, 10.0f, 4087.0f, 12.0f };
        SkRect dstR2 = { 10, 410, 30, 430 };

        if (!fUseIRect) {
            canvas->drawBitmapRectToRect(fBigBitmap, &srcR1, dstR1, &paint);
            canvas->drawBitmapRectToRect(fBigBitmap, &srcR2, dstR2, &paint);
        } else {
            SkIRect iSrcR1, iSrcR2;

            srcR1.roundOut(&iSrcR1);
            srcR2.roundOut(&iSrcR2);

            canvas->drawBitmapRect(fBigBitmap, &iSrcR1, dstR1, &paint);
            canvas->drawBitmapRect(fBigBitmap, &iSrcR2, dstR2, &paint);
        }
    }

private:
    typedef skiagm::GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static skiagm::GM* MyFactory0(void*) { return new DrawBitmapRect2(false); }
static skiagm::GM* MyFactory1(void*) { return new DrawBitmapRect2(true); }

static skiagm::GM* MyFactory2(void*) { return new DrawBitmapRect3(); }

#ifndef SK_BUILD_FOR_ANDROID
static skiagm::GM* MyFactory3(void*) { return new DrawBitmapRect4(false); }
static skiagm::GM* MyFactory4(void*) { return new DrawBitmapRect4(true); }
#endif

static skiagm::GMRegistry reg0(MyFactory0);
static skiagm::GMRegistry reg1(MyFactory1);

static skiagm::GMRegistry reg2(MyFactory2);

#ifndef SK_BUILD_FOR_ANDROID
static skiagm::GMRegistry reg3(MyFactory3);
static skiagm::GMRegistry reg4(MyFactory4);
#endif
