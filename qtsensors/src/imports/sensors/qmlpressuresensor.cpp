/****************************************************************************
**
** Copyright (C) 2013 Research In Motion
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
#include "qmlpressuresensor.h"
#include <QtSensors/QPressureSensor>

/*!
    \qmltype PressureSensor
    \instantiates QmlPressureSensor
    \ingroup qml-sensors_type
    \inqmlmodule QtSensors
    \since QtSensors 5.1
    \inherits Sensor
    \brief The PressureSensor element reports on atmospheric pressure values.

    The PressureSensor element reports on atmospheric pressure values.

    This element wraps the QPressureSensor class. Please see the documentation for
    QPressureSensor for details.

    \sa PressureReading
*/

QmlPressureSensor::QmlPressureSensor(QObject *parent)
    : QmlSensor(parent)
    , m_sensor(new QPressureSensor(this))
{
}

QmlPressureSensor::~QmlPressureSensor()
{
}

QmlSensorReading *QmlPressureSensor::createReading() const
{
    return new QmlPressureReading(m_sensor);
}

QSensor *QmlPressureSensor::sensor() const
{
    return m_sensor;
}

/*!
    \qmltype PressureReading
    \instantiates QmlPressureReading
    \ingroup qml-sensors_reading
    \inqmlmodule QtSensors
    \since QtSensors 5.1
    \inherits SensorReading
    \brief The PressureReading element holds the most recent PressureSensor reading.

    The PressureReading element holds the most recent PressureSensor reading.

    This element wraps the QPressureReading class. Please see the documentation for
    QPressureReading for details.

    This element cannot be directly created.
*/

QmlPressureReading::QmlPressureReading(QPressureSensor *sensor)
    : QmlSensorReading(sensor)
    , m_sensor(sensor)
    , m_pressure(0)
    , m_temperature(0)
{
}

QmlPressureReading::~QmlPressureReading()
{
}

/*!
    \qmlproperty qreal PressureReading::pressure
    This property holds the atmospheric pressure value in Pascals.

    Please see QPressureReading::pressure for information about this property.
*/

qreal QmlPressureReading::pressure() const
{
    return m_pressure;
}

/*!
    \qmlproperty qreal PressureReading::temperature
    This property holds the pressure sensor's temperature value in degrees Celsius.

    Please see QPressureReading::temperature for information about this property.
    \since QtSensors 5.2
*/

qreal QmlPressureReading::temperature() const
{
    return m_temperature;
}

QSensorReading *QmlPressureReading::reading() const
{
    return m_sensor->reading();
}

void QmlPressureReading::readingUpdate()
{
    qreal pressure = m_sensor->reading()->pressure();
    if (m_pressure != pressure) {
        m_pressure = pressure;
        Q_EMIT pressureChanged();
    }

    qreal temperature = m_sensor->reading()->temperature();
    if (m_temperature != temperature) {
        m_temperature = temperature;
        Q_EMIT temperatureChanged();
    }
}
