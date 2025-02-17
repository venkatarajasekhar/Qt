
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "SampleCode.h"
#include "SkView.h"
#include "SkCanvas.h"
#include "SkGradientShader.h"


class TwoPtRadialView : public SampleView {
public:
    TwoPtRadialView() {}

protected:
    // overrides from SkEventSink
    virtual bool onQuery(SkEvent* evt) {
        if (SampleCode::TitleQ(*evt)) {
            SampleCode::TitleR(evt, "2PtRadial");
            return true;
        }
        return this->INHERITED::onQuery(evt);
    }

    virtual void onDrawContent(SkCanvas* canvas) {
        canvas->translate(SkIntToScalar(10), SkIntToScalar(20));

        SkColor colors[] = { SK_ColorRED, SK_ColorBLUE };
        SkPoint c0 = { 0, 0 };
        SkScalar r0 = 100;
        SkPoint c1 = { 100, 100 };
        SkScalar r1 = 100;
        SkShader* s = SkGradientShader::CreateTwoPointRadial(c0, r0, c1, r1, colors,
                                                             NULL, 2,
                                                             SkShader::kClamp_TileMode);

        SkPaint paint;
        paint.setShader(s)->unref();
        canvas->drawPaint(paint);
    }

private:
    typedef SampleView INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static SkView* MyFactory() { return new TwoPtRadialView; }
static SkViewRegister reg(MyFactory);
