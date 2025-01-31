/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKCAMERASERVICE_H
#define MOCKCAMERASERVICE_H

#include "qmediaservice.h"
#include "../qmultimedia_common/mockcameraflashcontrol.h"
#include "../qmultimedia_common/mockcameralockscontrol.h"
#include "../qmultimedia_common/mockcamerafocuscontrol.h"
#include "../qmultimedia_common/mockcamerazoomcontrol.h"
#include "../qmultimedia_common/mockcameraimageprocessingcontrol.h"
#include "../qmultimedia_common/mockcameraimagecapturecontrol.h"
#include "../qmultimedia_common/mockcameraexposurecontrol.h"
#include "../qmultimedia_common/mockcameracapturedestinationcontrol.h"
#include "../qmultimedia_common/mockcameracapturebuffercontrol.h"
#include "../qmultimedia_common/mockimageencodercontrol.h"
#include "../qmultimedia_common/mockcameracontrol.h"
#include "../qmultimedia_common/mockvideosurface.h"
#include "../qmultimedia_common/mockvideorenderercontrol.h"
#include "../qmultimedia_common/mockvideowindowcontrol.h"
#include "../qmultimedia_common/mockvideodeviceselectorcontrol.h"
#include "../qmultimedia_common/mockcamerainfocontrol.h"

class MockSimpleCameraService : public QMediaService
{
    Q_OBJECT

public:
    MockSimpleCameraService(): QMediaService(0)
    {
        mockControl = new MockCameraControl(this);
    }

    ~MockSimpleCameraService()
    {
    }

    QMediaControl* requestControl(const char *iid)
    {
        if (qstrcmp(iid, QCameraControl_iid) == 0)
            return mockControl;
        return 0;
    }

    void releaseControl(QMediaControl*) {}

    MockCameraControl *mockControl;
};


class MockCameraService : public QMediaService
{
    Q_OBJECT

public:
    MockCameraService(): QMediaService(0)
    {
        mockControl = new MockCameraControl(this);
        mockLocksControl = new MockCameraLocksControl(this);
        mockExposureControl = new MockCameraExposureControl(this);
        mockFlashControl = new MockCameraFlashControl(this);
        mockFocusControl = new MockCameraFocusControl(this);
        mockZoomControl = new MockCameraZoomControl(this);
        mockCaptureControl = new MockCaptureControl(mockControl, this);
        mockCaptureBufferControl = new MockCaptureBufferFormatControl(this);
        mockCaptureDestinationControl = new MockCaptureDestinationControl(this);
        mockImageProcessingControl = new MockImageProcessingControl(this);
        mockImageEncoderControl = new MockImageEncoderControl(this);
        rendererControl = new MockVideoRendererControl(this);
        windowControl = new MockVideoWindowControl(this);
        mockVideoDeviceSelectorControl = new MockVideoDeviceSelectorControl(this);
        mockCameraInfoControl = new MockCameraInfoControl(this);
        rendererRef = 0;
        windowRef = 0;
    }

    ~MockCameraService()
    {
    }

    QMediaControl* requestControl(const char *iid)
    {
        if (qstrcmp(iid, QCameraControl_iid) == 0)
            return mockControl;

        if (qstrcmp(iid, QCameraLocksControl_iid) == 0)
            return mockLocksControl;

        if (qstrcmp(iid, QCameraExposureControl_iid) == 0)
            return mockExposureControl;

        if (qstrcmp(iid, QCameraFlashControl_iid) == 0)
            return mockFlashControl;

        if (qstrcmp(iid, QCameraFocusControl_iid) == 0)
            return mockFocusControl;

        if (qstrcmp(iid, QCameraZoomControl_iid) == 0)
            return mockZoomControl;

        if (qstrcmp(iid, QCameraImageCaptureControl_iid) == 0)
            return mockCaptureControl;

        if (qstrcmp(iid, QCameraCaptureBufferFormatControl_iid) == 0)
            return mockCaptureBufferControl;

        if (qstrcmp(iid, QCameraCaptureDestinationControl_iid) == 0)
            return mockCaptureDestinationControl;

        if (qstrcmp(iid, QCameraImageProcessingControl_iid) == 0)
            return mockImageProcessingControl;

        if (qstrcmp(iid, QImageEncoderControl_iid) == 0)
            return mockImageEncoderControl;

        if (qstrcmp(iid, QVideoDeviceSelectorControl_iid) == 0)
            return mockVideoDeviceSelectorControl;

        if (qstrcmp(iid, QCameraInfoControl_iid) == 0)
            return mockCameraInfoControl;

        if (qstrcmp(iid, QVideoRendererControl_iid) == 0) {
            if (rendererRef == 0) {
                rendererRef += 1;
                return rendererControl;
            }
        }
        if (qstrcmp(iid, QVideoWindowControl_iid) == 0) {
            if (windowRef == 0) {
                windowRef += 1;
                return windowControl;
            }
        }
        return 0;
    }

    void releaseControl(QMediaControl *control)
    {
        if (control == rendererControl)
            rendererRef -= 1;
        if (control == windowControl)
            windowRef -= 1;
    }

    MockCameraControl *mockControl;
    MockCameraLocksControl *mockLocksControl;
    MockCaptureControl *mockCaptureControl;
    MockCaptureBufferFormatControl *mockCaptureBufferControl;
    MockCaptureDestinationControl *mockCaptureDestinationControl;
    MockCameraExposureControl *mockExposureControl;
    MockCameraFlashControl *mockFlashControl;
    MockCameraFocusControl *mockFocusControl;
    MockCameraZoomControl *mockZoomControl;
    MockImageProcessingControl *mockImageProcessingControl;
    MockImageEncoderControl *mockImageEncoderControl;
    MockVideoRendererControl *rendererControl;
    MockVideoWindowControl *windowControl;
    MockVideoDeviceSelectorControl *mockVideoDeviceSelectorControl;
    MockCameraInfoControl *mockCameraInfoControl;
    int rendererRef;
    int windowRef;
};

#endif // MOCKCAMERASERVICE_H
