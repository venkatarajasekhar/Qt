/*
 * Copyright 2013 Google Inc.
 *
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef SkV8Example_DEFINED
#define SkV8Example_DEFINED

#include "SkWindow.h"

class GrContext;
class GrGLInterface;
class GrRenderTarget;
class SkSurface;

class JsContext;

class SkV8ExampleWindow : public SkOSWindow {
public:
    SkV8ExampleWindow(void* hwnd, JsContext* canvas);
    virtual ~SkV8ExampleWindow();

protected:
    virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE;
    virtual void onSizeChange() SK_OVERRIDE;

#if SK_SUPPORT_GPU
    virtual SkCanvas* createCanvas() SK_OVERRIDE;
#endif

#ifdef SK_BUILD_FOR_WIN
    virtual void onHandleInval(const SkIRect&) SK_OVERRIDE;
#endif

    void windowSizeChanged();

private:
    typedef SkOSWindow INHERITED;
    JsContext* fJsContext;

#if SK_SUPPORT_GPU
    GrContext*              fCurContext;
    const GrGLInterface*    fCurIntf;
    GrRenderTarget*         fCurRenderTarget;
    SkSurface*              fCurSurface;
#endif
};

#endif
