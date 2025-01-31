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

#include "qmlmagnetometer.h"
#include <QtSensors/QMagnetometer>

/*!
    \qmltype Magnetometer
    \instantiates QmlMagnetometer
    \ingroup qml-sensors_type
    \inqmlmodule QtSensors
    \since QtSensors 5.0
    \inherits Sensor
    \brief The Magnetometer element reports on magnetic field strength
           along the Z, Y and Z axes.

    The Magnetometer element reports on magnetic field strength
    along the Z, Y and Z axes.

    This element wraps the QMagnetometer class. Please see the documentation for
    QMagnetometer for details.

    \sa MagnetometerReading
*/

QmlMagnetometer::QmlMagnetometer(QObject *parent)
    : QmlSensor(parent)
    , m_sensor(new QMagnetometer(this))
{
    connect(m_sensor, SIGNAL(returnGeoValuesChanged(bool)),
            this, SIGNAL(returnGeoValuesChanged(bool)));
}

QmlMagnetometer::~QmlMagnetometer()
{
}

QmlSensorReading *QmlMagnetometer::createReading() const
{
    return new QmlMagnetometerReading(m_sensor);
}

QSensor *QmlMagnetometer::sensor() const
{
    return m_sensor;
}

/*!
    \qmlproperty bool Magnetometer::returnGeoValues
    This property holds a value indicating if geomagnetic values should be returned.

    Please see QMagnetometer::returnGeoValues for information about this property.
*/

bool QmlMagnetometer::returnGeoValues() const
{
    return m_sensor->returnGeoValues();
}

void QmlMagnetometer::setReturnGeoValues(bool geo)
{
    m_sensor->setReturnGeoValues(geo);
}

/*!
    \qmltype MagnetometerReading
    \instantiates QmlMagnetometerReading
    \ingroup qml-sensors_reading
    \inqmlmodule QtSensors
    \since QtSensors 5.0
    \inherits SensorReading
    \brief The MagnetometerReading element holds the most recent Magnetometer reading.

    The MagnetometerReading element holds the most recent Magnetometer reading.

    This element wraps the QMagnetometerReading class. Please see the documentation for
    QMagnetometerReading for details.

    This element cannot be directly created.
*/

QmlMagnetometerReading::QmlMagnetometerReading(QMagnetometer *sensor)
    : QmlSensorReading(sensor)
    , m_sensor(sensor)
{
}

QmlMagnetometerReading::~QmlMagnetometerReading()
{
}

/*!
    \qmlproperty qreal MagnetometerReading::x
    This property holds the raw magnetic flux density on the X axis.

    Please see QMagnetometerReading::x for information about this property.
*/

qreal QmlMagnetometerReading::x() const
{
    return m_x;
}

/*!
    \qmlproperty qreal MagnetometerReading::y
    This property holds the raw magnetic flux density on the Y axis.

    Please see QMagnetometerReading::y for information about this property.
*/

qreal QmlMagnetometerReading::y() const
{
    return m_y;
}

/*!
    \qmlproperty qreal MagnetometerReading::z
    This property holds the raw magnetic flux density on the Z axis.

    Please see QMagnetometerReading::z for information about this property.
*/

qreal QmlMagnetometerReading::z() const
{
    return m_z;
}

/*!
    \qmlproperty qreal MagnetometerReading::calibrationLevel
    This property holds the accuracy of the reading.

    Please see QMagnetometerReading::calibrationLevel for information about this property.
*/

qreal QmlMagnetometerReading::calibrationLevel() const
{
    return m_calibrationLevel;
}

QSensorReading *QmlMagnetometerReading::reading() const
{
    return m_sensor->reading();
}

void QmlMagnetometerReading::readingUpdate()
{
    qreal magX = m_sensor->reading()->x();
    if (m_x != magX) {
        m_x = magX;
        Q_EMIT xChanged();
    }
    qreal magY = m_sensor->reading()->y();
    if (m_y != magY) {
        m_y = magY;
        Q_EMIT yChanged();
    }
    qreal magZ = m_sensor->reading()->z();
    if (m_z != magZ) {
        m_z = magZ;
        Q_EMIT zChanged();
    }
    qreal calLevel = m_sensor->reading()->calibrationLevel();
    if (m_calibrationLevel != calLevel) {
        m_calibrationLevel = calLevel;
        Q_EMIT calibrationLevelChanged();
    }
}
