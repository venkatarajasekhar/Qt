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

#ifndef MOCKCAMERACONTROL_H
#define MOCKCAMERACONTROL_H

#include "qcameracontrol.h"

class MockCameraControl : public QCameraControl
{
    friend class MockCaptureControl;
    Q_OBJECT
public:
    MockCameraControl(QObject *parent = 0):
            QCameraControl(parent),
            m_state(QCamera::UnloadedState),
            m_captureMode(QCamera::CaptureStillImage),
            m_status(QCamera::UnloadedStatus),
            m_propertyChangesSupported(false)
    {
    }

    ~MockCameraControl() {}

    void start() { m_state = QCamera::ActiveState; }
    virtual void stop() { m_state = QCamera::UnloadedState; }
    QCamera::State state() const { return m_state; }
    void setState(QCamera::State state) {
        if (m_state != state) {
            m_state = state;

            switch (state) {
            case QCamera::UnloadedState:
                m_status = QCamera::UnloadedStatus;
                break;
            case QCamera::LoadedState:
                m_status = QCamera::LoadedStatus;
                break;
            case QCamera::ActiveState:
                m_status = QCamera::ActiveStatus;
                break;
            default:
                emit error(QCamera::NotSupportedFeatureError, "State not supported.");
                return;
            }

            emit stateChanged(m_state);
            emit statusChanged(m_status);
        }
    }

    QCamera::Status status() const { return m_status; }

    QCamera::CaptureModes captureMode() const { return m_captureMode; }
    void setCaptureMode(QCamera::CaptureModes mode)
    {
        if (m_captureMode != mode) {
            if (m_state == QCamera::ActiveState && !m_propertyChangesSupported)
                return;
            m_captureMode = mode;
            emit captureModeChanged(mode);
        }
    }

    bool isCaptureModeSupported(QCamera::CaptureModes mode) const
    {
        return mode == QCamera::CaptureStillImage || mode == QCamera::CaptureVideo;
    }

    QCamera::LockTypes supportedLocks() const
    {
        return QCamera::LockExposure | QCamera::LockFocus | QCamera::LockWhiteBalance;
    }

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const
    {
        Q_UNUSED(status);
        if (changeType == QCameraControl::ImageEncodingSettings && m_captureMode == QCamera::CaptureVideo)
            return true;
        else if (changeType== QCameraControl::VideoEncodingSettings)
            return true;
        else
            return m_propertyChangesSupported;
    }

    /* helper method to emit the signal error */
    void setError(QCamera::Error err, QString errorString)
    {
        emit error(err, errorString);
    }

    /* helper method to emit the signal statusChaged */
    void setStatus(QCamera::Status newStatus)
    {
        m_status = newStatus;
        emit statusChanged(newStatus);
    }

    QCamera::State m_state;
    QCamera::CaptureModes m_captureMode;
    QCamera::Status m_status;
    bool m_propertyChangesSupported;
};



#endif // MOCKCAMERACONTROL_H
