/*
 * Copyright 2011 Skia
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SampleApp_DEFINED
#define SampleApp_DEFINED

#include "SkOSMenu.h"
#include "SkPath.h"
#include "SkPicture.h"
#include "SkPictureRecorder.h"
#include "SkScalar.h"
#include "SkTDArray.h"
#include "SkTouchGesture.h"
#include "SkWindow.h"

class GrContext;
class GrRenderTarget;

class SkCanvas;
class SkData;
class SkEvent;
class SkTypeface;
class SkViewFactory;

class SampleWindow : public SkOSWindow {
    SkTDArray<const SkViewFactory*> fSamples;
public:
    enum DeviceType {
        kRaster_DeviceType,
        kPicture_DeviceType,
#if SK_SUPPORT_GPU
        kGPU_DeviceType,
#if SK_ANGLE
        kANGLE_DeviceType,
#endif // SK_ANGLE
        kNullGPU_DeviceType,
#endif // SK_SUPPORT_GPU

        kDeviceTypeCnt
    };

    static bool IsGpuDeviceType(DeviceType devType) {
    #if SK_SUPPORT_GPU
        switch (devType) {
            case kGPU_DeviceType:
    #if SK_ANGLE
            case kANGLE_DeviceType:
    #endif // SK_ANGLE
            case kNullGPU_DeviceType:
                return true;
            default:
                return false;
        }
    #endif // SK_SUPPORT_GPU
        return false;
    }

    /**
     * SampleApp ports can subclass this manager class if they want to:
     *      * filter the types of devices supported
     *      * customize plugging of SkBaseDevice objects into an SkCanvas
     *      * customize publishing the results of draw to the OS window
     *      * manage GrContext / GrRenderTarget lifetimes
     */
    class DeviceManager : public SkRefCnt {
    public:
        SK_DECLARE_INST_COUNT(DeviceManager)

        virtual void setUpBackend(SampleWindow* win, int msaaSampleCount) = 0;

        virtual void tearDownBackend(SampleWindow* win) = 0;

        // called before drawing. should install correct device
        // type on the canvas. Will skip drawing if returns false.
        virtual SkCanvas* createCanvas(DeviceType dType, SampleWindow* win) = 0;

        // called after drawing, should get the results onto the
        // screen.
        virtual void publishCanvas(DeviceType dType,
                                   SkCanvas* canvas,
                                   SampleWindow* win) = 0;

        // called when window changes size, guaranteed to be called
        // at least once before first draw (after init)
        virtual void windowSizeChanged(SampleWindow* win) = 0;

        // return the GrContext backing gpu devices (NULL if not built with GPU support)
        virtual GrContext* getGrContext() = 0;

        // return the GrRenderTarget backing gpu devices (NULL if not built with GPU support)
        virtual GrRenderTarget* getGrRenderTarget() = 0;
    private:
        typedef SkRefCnt INHERITED;
    };

    SampleWindow(void* hwnd, int argc, char** argv, DeviceManager*);
    virtual ~SampleWindow();

    virtual SkCanvas* createCanvas() SK_OVERRIDE {
        SkCanvas* canvas = NULL;
        if (fDevManager) {
            canvas = fDevManager->createCanvas(fDeviceType, this);
        }
        if (NULL == canvas) {
            canvas = this->INHERITED::createCanvas();
        }
        return canvas;
    }

    virtual void draw(SkCanvas* canvas);

    void setDeviceType(DeviceType type);
    void toggleRendering();
    void toggleSlideshow();
    void toggleFPS();
    void showOverview();

    GrContext* getGrContext() const { return fDevManager->getGrContext(); }

    void setZoomCenter(float x, float y);
    void changeZoomLevel(float delta);
    bool nextSample();
    bool previousSample();
    bool goToSample(int i);
    SkString getSampleTitle(int i);
    int  sampleCount();
    bool handleTouch(int ownerId, float x, float y,
            SkView::Click::State state);
    void saveToPdf();
    SkData* getPDFData() { return fPDFData; }
    void postInvalDelay();

    DeviceType getDeviceType() const { return fDeviceType; }

protected:
    virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE;
    virtual bool onHandleKey(SkKey key) SK_OVERRIDE;
    virtual bool onHandleChar(SkUnichar) SK_OVERRIDE;
    virtual void onSizeChange() SK_OVERRIDE;

    virtual SkCanvas* beforeChildren(SkCanvas*) SK_OVERRIDE;
    virtual void afterChildren(SkCanvas*) SK_OVERRIDE;
    virtual void beforeChild(SkView* child, SkCanvas* canvas) SK_OVERRIDE;
    virtual void afterChild(SkView* child, SkCanvas* canvas) SK_OVERRIDE;

    virtual bool onEvent(const SkEvent& evt) SK_OVERRIDE;
    virtual bool onQuery(SkEvent* evt) SK_OVERRIDE;

    virtual bool onDispatchClick(int x, int y, Click::State, void* owner,
                                 unsigned modi) SK_OVERRIDE;
    virtual bool onClick(Click* click) SK_OVERRIDE;
    virtual Click* onFindClickHandler(SkScalar x, SkScalar y,
                                      unsigned modi) SK_OVERRIDE;

private:
    class DefaultDeviceManager;

    int fCurrIndex;

    SkPictureRecorder fRecorder;
    SkPath fClipPath;

    SkTouchGesture fGesture;
    SkScalar fZoomLevel;
    SkScalar fZoomScale;

    DeviceType fDeviceType;
    DeviceManager* fDevManager;

    bool fSaveToPdf;
    SkCanvas* fPdfCanvas;
    SkData* fPDFData;

    bool fUseClip;
    bool fNClip;
    bool fAnimating;
    bool fRotate;
    SkScalar fRotateAnimTime;
    bool fPerspAnim;
    SkScalar fPerspAnimTime;
    bool fRequestGrabImage;
    bool fMeasureFPS;
    SkMSec fMeasureFPS_Time;
    SkMSec fMeasureFPS_StartTime;
    bool fMagnify;
    int fTilingMode;


    SkOSMenu::TriState fPipeState;  // Mixed uses a tiled pipe
                                    // On uses a normal pipe
                                    // Off uses no pipe
    int  fUsePipeMenuItemID;

    // The following are for the 'fatbits' drawing
    // Latest position of the mouse.
    int fMouseX, fMouseY;
    int fFatBitsScale;
    // Used by the text showing position and color values.
    SkTypeface* fTypeface;
    bool fShowZoomer;

    SkOSMenu::TriState fLCDState;
    SkOSMenu::TriState fAAState;
    SkOSMenu::TriState fSubpixelState;
    int fHintingState;
    int fFilterLevelIndex;
    unsigned   fFlipAxis;

    int fMSAASampleCount;

    int fScrollTestX, fScrollTestY;
    SkScalar fZoomCenterX, fZoomCenterY;

    //Stores global settings
    SkOSMenu* fAppMenu; // We pass ownership to SkWindow, when we call addMenu
    //Stores slide specific settings
    SkOSMenu* fSlideMenu; // We pass ownership to SkWindow, when we call addMenu

    int fTransitionNext;
    int fTransitionPrev;

    void loadView(SkView*);
    void updateTitle();

    bool zoomIn();
    bool zoomOut();
    void updatePointer(int x, int y);
    void magnify(SkCanvas* canvas);
    void showZoomer(SkCanvas* canvas);
    void updateMatrix();
    void postAnimatingEvent();
    void installDrawFilter(SkCanvas*);
    int findByTitle(const char*);
    void listTitles();
    SkSize tileSize() const;

    typedef SkOSWindow INHERITED;
};

#endif
