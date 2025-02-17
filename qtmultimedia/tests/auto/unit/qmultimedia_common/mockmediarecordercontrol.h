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

#ifndef MOCKRECORDERCONTROL_H
#define MOCKRECORDERCONTROL_H

#include <QUrl>

#include "qmediarecordercontrol.h"

class MockMediaRecorderControl : public QMediaRecorderControl
{
    Q_OBJECT

public:
    MockMediaRecorderControl(QObject *parent = 0):
        QMediaRecorderControl(parent),
        m_state(QMediaRecorder::StoppedState),
        m_status(QMediaRecorder::LoadedStatus),
        m_position(0),
        m_muted(false),
        m_volume(1.0),
        m_settingAppliedCount(0)
    {
    }

    QUrl outputLocation() const
    {
        return m_sink;
    }

    bool setOutputLocation(const QUrl &sink)
    {
        m_sink = sink;
        return true;
    }

    QMediaRecorder::State state() const
    {
        return m_state;
    }

    QMediaRecorder::Status status() const
    {
        return m_status;
    }

    qint64 duration() const
    {
        return m_position;
    }

    bool isMuted() const
    {
        return m_muted;
    }

    qreal volume() const
    {
        return m_volume;
    }

    void applySettings()
    {
        m_settingAppliedCount++;
    }

    using QMediaRecorderControl::error;

public slots:
    void record()
    {
        m_state = QMediaRecorder::RecordingState;
        m_status = QMediaRecorder::RecordingStatus;
        m_position=1;
        emit stateChanged(m_state);
        emit statusChanged(m_status);
        emit durationChanged(m_position);

        QUrl actualLocation = m_sink.isEmpty() ? QUrl::fromLocalFile("default_name.mp4") : m_sink;
        emit actualLocationChanged(actualLocation);
    }

    void pause()
    {
        m_state = QMediaRecorder::PausedState;
        m_status = QMediaRecorder::PausedStatus;
        emit stateChanged(m_state);
        emit statusChanged(m_status);
    }

    void stop()
    {
        m_position=0;
        m_state = QMediaRecorder::StoppedState;
        m_status = QMediaRecorder::LoadedStatus;
        emit stateChanged(m_state);
        emit statusChanged(m_status);
    }

    void setState(QMediaRecorder::State state)
    {
        switch (state) {
        case QMediaRecorder::StoppedState:
            stop();
            break;
        case QMediaRecorder::PausedState:
            pause();
            break;
        case QMediaRecorder::RecordingState:
            record();
            break;
        }
    }


    void setMuted(bool muted)
    {
        if (m_muted != muted)
            emit mutedChanged(m_muted = muted);
    }

    void setVolume(qreal volume)
    {
        if (!qFuzzyCompare(m_volume, volume))
            emit volumeChanged(m_volume = volume);
    }

public:
    QUrl       m_sink;
    QMediaRecorder::State m_state;
    QMediaRecorder::Status m_status;
    qint64     m_position;
    bool m_muted;
    qreal m_volume;
    int m_settingAppliedCount;
};

#endif // MOCKRECORDERCONTROL_H
