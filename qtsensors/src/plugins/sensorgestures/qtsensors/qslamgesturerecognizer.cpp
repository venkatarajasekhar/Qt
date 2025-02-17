/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSensors module of the Qt Toolkit.
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

#include "qslamgesturerecognizer.h"
#include "qtsensorgesturesensorhandler.h"


#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

QSlamSensorGestureRecognizer::QSlamSensorGestureRecognizer(QObject *parent) :
    QSensorGestureRecognizer(parent),
    orientationReading(0),
    accelRange(0),
    active(0),
    lastX(0),
    lastY(0),
    lastZ(0),
    detectedX(0),
    detecting(0),
    accelX(0),
    roll(0),
    resting(0),
    lastTimestamp(0),
    lapsedTime(0),
    timerActive(0)
{
}

QSlamSensorGestureRecognizer::~QSlamSensorGestureRecognizer()
{
}

void QSlamSensorGestureRecognizer::create()
{
}


QString QSlamSensorGestureRecognizer::id() const
{
    return QString("QtSensors.slam");
}

bool QSlamSensorGestureRecognizer::start()
{
    if (QtSensorGestureSensorHandler::instance()->startSensor(QtSensorGestureSensorHandler::Accel)) {
        if (QtSensorGestureSensorHandler::instance()->startSensor(QtSensorGestureSensorHandler::Orientation)) {
            active = true;
            accelRange = QtSensorGestureSensorHandler::instance()->accelRange;
            connect(QtSensorGestureSensorHandler::instance(),SIGNAL(orientationReadingChanged(QOrientationReading*)),
                    this,SLOT(orientationReadingChanged(QOrientationReading*)));

            connect(QtSensorGestureSensorHandler::instance(),SIGNAL(accelReadingChanged(QAccelerometerReading*)),
                    this,SLOT(accelChanged(QAccelerometerReading*)));
        } else {
            QtSensorGestureSensorHandler::instance()->stopSensor(QtSensorGestureSensorHandler::Accel);
            active = false;
        }
    } else {
        active = false;
    }
    return active;
}

bool QSlamSensorGestureRecognizer::stop()
{
    QtSensorGestureSensorHandler::instance()->stopSensor(QtSensorGestureSensorHandler::Accel);
    QtSensorGestureSensorHandler::instance()->stopSensor(QtSensorGestureSensorHandler::Orientation);
    disconnect(QtSensorGestureSensorHandler::instance(),SIGNAL(orientationReadingChanged(QOrientationReading*)),
            this,SLOT(orientationReadingChanged(QOrientationReading*)));

    disconnect(QtSensorGestureSensorHandler::instance(),SIGNAL(accelReadingChanged(QAccelerometerReading*)),
            this,SLOT(accelChanged(QAccelerometerReading*)));
    detecting = false;
    restingList.clear();
    active = false;
    return active;
}

bool QSlamSensorGestureRecognizer::isActive()
{
    return active;
}

void QSlamSensorGestureRecognizer::orientationReadingChanged(QOrientationReading *reading)
{
    orientationReading = reading;
}

#define SLAM_DETECTION_FACTOR 0.3 // 11.7
#define SLAM_RESTING_FACTOR 2.5
#define SLAM_RESTING_COUNT 5
#define SLAM_ZERO_FACTOR .02

void QSlamSensorGestureRecognizer::accelChanged(QAccelerometerReading *reading)
{
    const qreal x = reading->x();
    const qreal y = reading->y();
    const qreal z = reading->z();
    quint64 timestamp = reading->timestamp();

    if (qAbs(lastX - x) < SLAM_RESTING_FACTOR
            && qAbs(lastY - y) < SLAM_RESTING_FACTOR
            && qAbs(lastZ - z) < SLAM_RESTING_FACTOR) {
        resting = true;
    } else {
        resting = false;
    }

    if (restingList.count() > SLAM_RESTING_COUNT)
        restingList.removeLast();
    restingList.insert(0, resting);


    if (timerActive && lastTimestamp > 0)
        lapsedTime += (timestamp - lastTimestamp )/1000;

    if (timerActive && lapsedTime >= 250) {
        doSlam();
    }
    lastTimestamp = timestamp;

    if (orientationReading == 0) {
        return;
    }

    const qreal difference = lastX - x;

    if (!detecting
            && orientationReading->orientation() == QOrientationReading::TopUp
            && resting
            && hasBeenResting()) {
        detectedX = x;
        // start of gesture
        detecting = true;
        if (difference > 0)
            wasNegative = false;
        else
            wasNegative = true;
        restingList.clear();
    }
    if (detecting
            && qAbs(difference) > (accelRange * SLAM_DETECTION_FACTOR)) {
        timerActive = true;
    }
    if (detecting &&
            (qAbs(difference) < SLAM_ZERO_FACTOR && qAbs(difference) > 0)) {
        detecting = false;
    }
    lastX = x;
    lastY = y;
    lastZ = z;
}

bool QSlamSensorGestureRecognizer::hasBeenResting()
{
    for (int i = 0; i < restingList.count() - 1; i++) {
        if (!restingList.at(i)) {
            return false;
        }
    }
    return true;
}

void QSlamSensorGestureRecognizer::doSlam()
{
    if (detecting && (orientationReading->orientation() == QOrientationReading::RightUp
            || orientationReading->orientation() == QOrientationReading::LeftUp)) {
        Q_EMIT slam();
        Q_EMIT detected("slam");
        restingList.clear();
        detecting = false;
    }
    timerActive = false;
    lapsedTime = 0;
}

QT_END_NAMESPACE
