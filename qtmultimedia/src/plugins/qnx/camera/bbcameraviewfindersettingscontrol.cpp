/****************************************************************************
**
** Copyright (C) 2013 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
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
#include "bbcameraviewfindersettingscontrol.h"

#include "bbcamerasession.h"

#include <QDebug>

QT_BEGIN_NAMESPACE

BbCameraViewfinderSettingsControl::BbCameraViewfinderSettingsControl(BbCameraSession *session, QObject *parent)
    : QCameraViewfinderSettingsControl(parent)
    , m_session(session)
{
}

bool BbCameraViewfinderSettingsControl::isViewfinderParameterSupported(ViewfinderParameter parameter) const
{
    switch (parameter) {
    case QCameraViewfinderSettingsControl::Resolution:
        return true;
    case QCameraViewfinderSettingsControl::PixelAspectRatio:
        return false;
    case QCameraViewfinderSettingsControl::MinimumFrameRate:
        return true;
    case QCameraViewfinderSettingsControl::MaximumFrameRate:
        return true;
    case QCameraViewfinderSettingsControl::PixelFormat:
        return true;
    default:
        return false;
    }
}

QVariant BbCameraViewfinderSettingsControl::viewfinderParameter(ViewfinderParameter parameter) const
{
    if (parameter == QCameraViewfinderSettingsControl::Resolution) {
        camera_error_t result = CAMERA_EOK;
        unsigned int width = 0;
        unsigned int height = 0;

        if (m_session->captureMode() & QCamera::CaptureStillImage) {
            result = camera_get_photovf_property(m_session->handle(), CAMERA_IMGPROP_WIDTH, &width,
                                                                      CAMERA_IMGPROP_HEIGHT, &height);
        } else if (m_session->captureMode() & QCamera::CaptureVideo) {
            result = camera_get_videovf_property(m_session->handle(), CAMERA_IMGPROP_WIDTH, &width,
                                                                      CAMERA_IMGPROP_HEIGHT, &height);
        }

        if (result != CAMERA_EOK) {
            qWarning() << "Unable to retrieve resolution of viewfinder:" << result;
            return QVariant();
        }

        return QSize(width, height);

    } else if (parameter == QCameraViewfinderSettingsControl::MinimumFrameRate) {
        camera_error_t result = CAMERA_EOK;
        double minimumFrameRate = 0;

        if (m_session->captureMode() & QCamera::CaptureStillImage)
            result = camera_get_photovf_property(m_session->handle(), CAMERA_IMGPROP_MINFRAMERATE, &minimumFrameRate);
        else if (m_session->captureMode() & QCamera::CaptureVideo)
            result = camera_get_videovf_property(m_session->handle(), CAMERA_IMGPROP_MINFRAMERATE, &minimumFrameRate);

        if (result != CAMERA_EOK) {
            qWarning() << "Unable to retrieve minimum framerate of viewfinder:" << result;
            return QVariant();
        }

        return QVariant(static_cast<qreal>(minimumFrameRate));

    } else if (parameter == QCameraViewfinderSettingsControl::MaximumFrameRate) {
        camera_error_t result = CAMERA_EOK;
        double maximumFrameRate = 0;

        if (m_session->captureMode() & QCamera::CaptureStillImage)
            result = camera_get_photovf_property(m_session->handle(), CAMERA_IMGPROP_FRAMERATE, &maximumFrameRate);
        else if (m_session->captureMode() & QCamera::CaptureVideo)
            result = camera_get_videovf_property(m_session->handle(), CAMERA_IMGPROP_FRAMERATE, &maximumFrameRate);

        if (result != CAMERA_EOK) {
            qWarning() << "Unable to retrieve maximum framerate of viewfinder:" << result;
            return QVariant();
        }

        return QVariant(static_cast<qreal>(maximumFrameRate));
    } else if (parameter == QCameraViewfinderSettingsControl::PixelFormat) {
        camera_error_t result = CAMERA_EOK;
        camera_frametype_t format = CAMERA_FRAMETYPE_UNSPECIFIED;

        if (m_session->captureMode() & QCamera::CaptureStillImage)
            result = camera_get_photovf_property(m_session->handle(), CAMERA_IMGPROP_FORMAT, &format);
        else if (m_session->captureMode() & QCamera::CaptureVideo)
            result = camera_get_videovf_property(m_session->handle(), CAMERA_IMGPROP_FORMAT, &format);

        if (result != CAMERA_EOK) {
            qWarning() << "Unable to retrieve pixel format of viewfinder:" << result;
            return QVariant();
        }

        switch (format) {
        case CAMERA_FRAMETYPE_UNSPECIFIED:
            return QVideoFrame::Format_Invalid;
        case CAMERA_FRAMETYPE_NV12:
            return QVideoFrame::Format_NV12;
        case CAMERA_FRAMETYPE_RGB8888:
            return QVideoFrame::Format_ARGB32;
        case CAMERA_FRAMETYPE_RGB888:
            return QVideoFrame::Format_RGB24;
        case CAMERA_FRAMETYPE_JPEG:
            return QVideoFrame::Format_Jpeg;
        case CAMERA_FRAMETYPE_GRAY8:
            return QVideoFrame::Format_Y8;
        case CAMERA_FRAMETYPE_METADATA:
            return QVideoFrame::Format_Invalid;
        case CAMERA_FRAMETYPE_BAYER:
            return QVideoFrame::Format_Invalid;
        case CAMERA_FRAMETYPE_CBYCRY:
            return QVideoFrame::Format_Invalid;
#ifndef Q_OS_BLACKBERRY_TABLET
        case CAMERA_FRAMETYPE_COMPRESSEDVIDEO:
            return QVideoFrame::Format_Invalid;
        case CAMERA_FRAMETYPE_COMPRESSEDAUDIO:
            return QVideoFrame::Format_Invalid;
#endif
        default:
            return QVideoFrame::Format_Invalid;
        }
    }

    return QVariant();
}

void BbCameraViewfinderSettingsControl::setViewfinderParameter(ViewfinderParameter parameter, const QVariant &value)
{
    if (parameter == QCameraViewfinderSettingsControl::Resolution) {
        camera_error_t result = CAMERA_EOK;
        const QSize size = value.toSize();

        if (m_session->captureMode() & QCamera::CaptureStillImage) {
            result = camera_set_photovf_property(m_session->handle(), CAMERA_IMGPROP_WIDTH, size.width(),
                                                                      CAMERA_IMGPROP_HEIGHT, size.height());
        } else if (m_session->captureMode() & QCamera::CaptureVideo) {
            result = camera_set_videovf_property(m_session->handle(), CAMERA_IMGPROP_WIDTH, size.width(),
                                                                      CAMERA_IMGPROP_HEIGHT, size.height());
        }

        if (result != CAMERA_EOK)
            qWarning() << "Unable to set resolution of viewfinder:" << result;

    } else if (parameter == QCameraViewfinderSettingsControl::MinimumFrameRate) {
        camera_error_t result = CAMERA_EOK;
        const double minimumFrameRate = value.toReal();

        if (m_session->captureMode() & QCamera::CaptureStillImage)
            result = camera_set_photovf_property(m_session->handle(), CAMERA_IMGPROP_MINFRAMERATE, minimumFrameRate);
        else if (m_session->captureMode() & QCamera::CaptureVideo)
            result = camera_set_videovf_property(m_session->handle(), CAMERA_IMGPROP_MINFRAMERATE, minimumFrameRate);

        if (result != CAMERA_EOK)
            qWarning() << "Unable to set minimum framerate of viewfinder:" << result;

    } else if (parameter == QCameraViewfinderSettingsControl::MaximumFrameRate) {
        camera_error_t result = CAMERA_EOK;
        const double maximumFrameRate = value.toReal();

        if (m_session->captureMode() & QCamera::CaptureStillImage)
            result = camera_set_photovf_property(m_session->handle(), CAMERA_IMGPROP_FRAMERATE, maximumFrameRate);
        else if (m_session->captureMode() & QCamera::CaptureVideo)
            result = camera_set_videovf_property(m_session->handle(), CAMERA_IMGPROP_FRAMERATE, maximumFrameRate);

        if (result != CAMERA_EOK)
            qWarning() << "Unable to set maximum framerate of viewfinder:" << result;

    } else if (parameter == QCameraViewfinderSettingsControl::PixelFormat) {
        camera_error_t result = CAMERA_EOK;
        camera_frametype_t format = CAMERA_FRAMETYPE_UNSPECIFIED;

        switch (value.value<QVideoFrame::PixelFormat>()) {
        case QVideoFrame::Format_NV12:
            format = CAMERA_FRAMETYPE_NV12;
            break;
        case QVideoFrame::Format_ARGB32:
            format = CAMERA_FRAMETYPE_RGB8888;
            break;
        case QVideoFrame::Format_RGB24:
            format = CAMERA_FRAMETYPE_RGB888;
            break;
        case QVideoFrame::Format_Jpeg:
            format = CAMERA_FRAMETYPE_JPEG;
            break;
        case QVideoFrame::Format_Y8:
            format = CAMERA_FRAMETYPE_GRAY8;
            break;
        default:
            format = CAMERA_FRAMETYPE_UNSPECIFIED;
            break;
        }

        if (m_session->captureMode() & QCamera::CaptureStillImage)
            result = camera_set_photovf_property(m_session->handle(), CAMERA_IMGPROP_FORMAT, format);
        else if (m_session->captureMode() & QCamera::CaptureVideo)
            result = camera_set_videovf_property(m_session->handle(), CAMERA_IMGPROP_FORMAT, format);

        if (result != CAMERA_EOK)
            qWarning() << "Unable to set pixel format of viewfinder:" << result;
    }
}

QT_END_NAMESPACE
