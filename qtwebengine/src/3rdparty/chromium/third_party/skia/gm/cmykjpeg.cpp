/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"

#include "Resources.h"
#include "SkCanvas.h"
#include "SkImageDecoder.h"
#include "SkStream.h"

namespace skiagm {

/** Draw a CMYK encoded jpeg - libjpeg doesn't support CMYK->RGB
    conversion so this tests Skia's internal processing
*/
class CMYKJpegGM : public GM {
public:
    CMYKJpegGM() {}

protected:
    virtual void onOnceBeforeDraw() SK_OVERRIDE {
        // parameters to the "decode" call
        bool dither = false;

        SkString resourcePath = GetResourcePath();
        if (!resourcePath.endsWith("/") && !resourcePath.endsWith("\\")) {
            resourcePath.append("/");
        }

        resourcePath.append("CMYK.jpg");

        SkFILEStream stream(resourcePath.c_str());
        if (!stream.isValid()) {
            SkDebugf("Could not find CMYK.jpg, please set --resourcePath correctly.\n");
            return;
        }

        SkImageDecoder* codec = SkImageDecoder::Factory(&stream);
        if (codec) {
            stream.rewind();
            codec->setDitherImage(dither);
            codec->decode(&stream, &fBitmap, kN32_SkColorType, SkImageDecoder::kDecodePixels_Mode);
            SkDELETE(codec);
        }
    }

    virtual SkString onShortName() {
        return SkString("cmykjpeg");
    }

    virtual SkISize onISize() {
        return SkISize::Make(640, 480);
    }

    virtual void onDraw(SkCanvas* canvas) {

        canvas->translate(20*SK_Scalar1, 20*SK_Scalar1);
        canvas->drawBitmap(fBitmap, 0, 0);
    }

private:
    SkBitmap fBitmap;

    typedef GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static GM* MyFactory(void*) { return new CMYKJpegGM; }
static GMRegistry reg(MyFactory);

}
