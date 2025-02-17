
/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// This test only works with the GPU backend.

#include "gm.h"

#if SK_SUPPORT_GPU

#include "GrContext.h"
#include "GrTest.h"
#include "effects/GrTextureDomain.h"
#include "SkBitmap.h"
#include "SkGr.h"
#include "SkGradientShader.h"

namespace skiagm {
/**
 * This GM directly exercises GrTextureDomainEffect.
 */
class TextureDomainEffect : public GM {
public:
    TextureDomainEffect() {
        this->setBGColor(0xFFFFFFFF);
    }

protected:
    virtual SkString onShortName() SK_OVERRIDE {
        return SkString("texture_domain_effect");
    }

    virtual SkISize onISize() SK_OVERRIDE {
        return SkISize::Make(400, 800);
    }

    virtual uint32_t onGetFlags() const SK_OVERRIDE {
        // This is a GPU-specific GM.
        return kGPUOnly_Flag;
    }

    virtual void onOnceBeforeDraw() SK_OVERRIDE {
        fBmp.allocN32Pixels(100, 100);
        SkCanvas canvas(fBmp);
        canvas.clear(0x00000000);
        SkPaint paint;

        SkColor colors1[] = { SK_ColorCYAN, SK_ColorLTGRAY, SK_ColorGRAY };
        paint.setShader(SkGradientShader::CreateSweep(65.f, 75.f, colors1,
                                                      NULL, SK_ARRAY_COUNT(colors1)))->unref();
        canvas.drawOval(SkRect::MakeXYWH(-5.f, -5.f,
                                         fBmp.width() + 10.f, fBmp.height() + 10.f), paint);

        SkColor colors2[] = { SK_ColorMAGENTA, SK_ColorLTGRAY, SK_ColorYELLOW };
        paint.setShader(SkGradientShader::CreateSweep(45.f, 55.f, colors2, NULL,
                                                      SK_ARRAY_COUNT(colors2)))->unref();
        paint.setXfermodeMode(SkXfermode::kDarken_Mode);
        canvas.drawOval(SkRect::MakeXYWH(-5.f, -5.f,
                                         fBmp.width() + 10.f, fBmp.height() + 10.f), paint);

        SkColor colors3[] = { SK_ColorBLUE, SK_ColorLTGRAY, SK_ColorGREEN };
        paint.setShader(SkGradientShader::CreateSweep(25.f, 35.f, colors3, NULL,
                                                      SK_ARRAY_COUNT(colors3)))->unref();
        paint.setXfermodeMode(SkXfermode::kLighten_Mode);
        canvas.drawOval(SkRect::MakeXYWH(-5.f, -5.f,
                                         fBmp.width() + 10.f, fBmp.height() + 10.f), paint);
    }

    virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE {
        GrRenderTarget* rt = canvas->internal_private_accessTopLayerRenderTarget();
        if (NULL == rt) {
            return;
        }
        GrContext* context = rt->getContext();
        if (NULL == context) {
            return;
        }

        GrTestTarget tt;
        context->getTestTarget(&tt);
        if (NULL == tt.target()) {
            SkDEBUGFAIL("Couldn't get Gr test target.");
            return;
        }

        GrDrawState* drawState = tt.target()->drawState();

        GrTexture* texture = GrLockAndRefCachedBitmapTexture(context, fBmp, NULL);
        if (NULL == texture) {
            return;
        }

        static const SkScalar kDrawPad = 10.f;
        static const SkScalar kTestPad = 10.f;

        SkTArray<SkMatrix> textureMatrices;
        textureMatrices.push_back().setIDiv(texture->width(), texture->height());
        textureMatrices.push_back() = textureMatrices[0];
        textureMatrices.back().postScale(1.5f, 0.85f);
        textureMatrices.push_back() = textureMatrices[0];
        textureMatrices.back().preRotate(45.f, texture->width() / 2.f, texture->height() / 2.f);

        const SkIRect texelDomains[] = {
            SkIRect::MakeWH(fBmp.width(), fBmp.height()),
            SkIRect::MakeXYWH(fBmp.width() / 4,
                              fBmp.height() / 4,
                              fBmp.width() / 2,
                              fBmp.height() / 2),
        };

        SkRect renderRect = SkRect::MakeWH(SkIntToScalar(fBmp.width()),
                                           SkIntToScalar(fBmp.height()));
        renderRect.outset(kDrawPad, kDrawPad);

        SkScalar y = kDrawPad + kTestPad;
        for (int tm = 0; tm < textureMatrices.count(); ++tm) {
            for (size_t d = 0; d < SK_ARRAY_COUNT(texelDomains); ++d) {
                SkScalar x = kDrawPad + kTestPad;
                for (int m = 0; m < GrTextureDomain::kModeCount; ++m) {
                    GrTextureDomain::Mode mode = (GrTextureDomain::Mode) m;
                    SkAutoTUnref<GrEffectRef> effect(
                        GrTextureDomainEffect::Create(texture, textureMatrices[tm],
                                                GrTextureDomain::MakeTexelDomain(texture,
                                                                                texelDomains[d]),
                                                mode, GrTextureParams::kNone_FilterMode));

                    if (!effect) {
                        continue;
                    }
                    SkMatrix viewMatrix;
                    viewMatrix.setTranslate(x, y);
                    drawState->reset(viewMatrix);
                    drawState->setRenderTarget(rt);
                    drawState->setColor(0xffffffff);
                    drawState->addColorEffect(effect, 1);

                    tt.target()->drawSimpleRect(renderRect);
                    x += renderRect.width() + kTestPad;
                }
                y += renderRect.height() + kTestPad;
            }
        }
        GrUnlockAndUnrefCachedBitmapTexture(texture);
    }

private:
    SkBitmap fBmp;

    typedef GM INHERITED;
};

DEF_GM( return SkNEW(TextureDomainEffect); )
}

#endif
