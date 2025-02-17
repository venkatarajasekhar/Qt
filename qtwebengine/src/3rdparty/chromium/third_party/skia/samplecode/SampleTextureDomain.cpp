/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SampleCode.h"
#include "SkBlurMask.h"
#include "SkBlurMaskFilter.h"
#include "SkCanvas.h"
#include "SkSurface.h"

static SkBitmap make_bitmap() {
    SkBitmap bm;
    bm.allocN32Pixels(5, 5);

    for (int y = 0; y < bm.height(); y++) {
        uint32_t* p = bm.getAddr32(0, y);
        for (int x = 0; x < bm.width(); x++) {
            p[x] = ((x + y) & 1) ? SK_ColorWHITE : SK_ColorBLACK;
        }
    }
    bm.unlockPixels();
    return bm;
}

class TextureDomainView : public SampleView {
    SkBitmap    fBM;

public:
    TextureDomainView(){
        fBM = make_bitmap();
    }

protected:
    // overrides from SkEventSink
    virtual bool onQuery(SkEvent* evt) {
        if (SampleCode::TitleQ(*evt)) {
            SampleCode::TitleR(evt, "Texture Domain");
            return true;
        }
        return this->INHERITED::onQuery(evt);
    }

    virtual void onDrawContent(SkCanvas* canvas) {
        SkRect srcRect;
        SkRect dstRect;
        SkPaint paint;
        paint.setFilterLevel(SkPaint::kLow_FilterLevel);

        // Test that bitmap draws from malloc-backed bitmaps respect
        // the constrained texture domain.
        srcRect.setXYWH(1, 1, 3, 3);
        dstRect.setXYWH(5, 5, 305, 305);
        canvas->drawBitmapRectToRect(fBM, &srcRect, dstRect, &paint);

        // Test that bitmap draws across separate devices also respect
        // the constrainted texture domain.
        // Note:  GPU-backed bitmaps follow a different rendering path
        // when copying from one GPU device to another.
        SkImageInfo info = SkImageInfo::MakeN32(5, 5, kOpaque_SkAlphaType);
        SkAutoTUnref<SkSurface> surface(canvas->newSurface(info));

        srcRect.setXYWH(1, 1, 3, 3);
        dstRect.setXYWH(1, 1, 3, 3);
        surface->getCanvas()->drawBitmapRectToRect(fBM, &srcRect, dstRect,
                                                   &paint);

        SkAutoTUnref<SkImage> image(surface->newImageSnapshot());

        srcRect.setXYWH(1, 1, 3, 3);
        dstRect.setXYWH(405, 5, 305, 305);
        image->draw(canvas, &srcRect, dstRect, &paint);

        // Test that bitmap blurring using a subrect
        // renders correctly
        srcRect.setXYWH(1, 1, 3, 3);
        dstRect.setXYWH(5, 405, 305, 305);
        SkMaskFilter* mf = SkBlurMaskFilter::Create(
            kNormal_SkBlurStyle,
            SkBlurMask::ConvertRadiusToSigma(SkIntToScalar(5)),
            SkBlurMaskFilter::kHighQuality_BlurFlag |
            SkBlurMaskFilter::kIgnoreTransform_BlurFlag);
        paint.setMaskFilter(mf)->unref();
        image->draw(canvas, &srcRect, dstRect, &paint);

        // Blur and a rotation + NULL src rect
        // This should not trigger the texture domain code
        // but it will test a code path in SkGpuDevice::drawBitmap
        // that handles blurs with rects transformed to non-
        // orthogonal rects. It also tests the NULL src rect handling
        mf = SkBlurMaskFilter::Create(kNormal_SkBlurStyle,
                                      SkBlurMask::ConvertRadiusToSigma(5),
                                      SkBlurMaskFilter::kHighQuality_BlurFlag);
        paint.setMaskFilter(mf)->unref();

        dstRect.setXYWH(-150, -150, 300, 300);
        canvas->translate(550, 550);
        canvas->rotate(45);
        canvas->drawBitmapRectToRect(fBM, NULL, dstRect, &paint);
    }
private:
    typedef SkView INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static SkView* MyFactory() { return new TextureDomainView; }
static SkViewRegister reg(MyFactory);
