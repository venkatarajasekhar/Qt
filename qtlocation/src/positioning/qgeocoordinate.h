/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtPositioning module of the Qt Toolkit.
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

#ifndef QGEOCOORDINATE_H
#define QGEOCOORDINATE_H

#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QSharedDataPointer>
#include <QtPositioning/qpositioningglobal.h>

QT_BEGIN_NAMESPACE

class QDebug;
class QDataStream;

class QGeoCoordinatePrivate;
class Q_POSITIONING_EXPORT QGeoCoordinate
{
public:
    enum CoordinateType {
        InvalidCoordinate,
        Coordinate2D,
        Coordinate3D
    };

    enum CoordinateFormat {
        Degrees,
        DegreesWithHemisphere,
        DegreesMinutes,
        DegreesMinutesWithHemisphere,
        DegreesMinutesSeconds,
        DegreesMinutesSecondsWithHemisphere
    };

    QGeoCoordinate();
    QGeoCoordinate(double latitude, double longitude);
    QGeoCoordinate(double latitude, double longitude, double altitude);
    QGeoCoordinate(const QGeoCoordinate &other);
    ~QGeoCoordinate();

    QGeoCoordinate &operator=(const QGeoCoordinate &other);

    bool operator==(const QGeoCoordinate &other) const;
    inline bool operator!=(const QGeoCoordinate &other) const {
        return !operator==(other);
    }

    bool isValid() const;
    CoordinateType type() const;

    void setLatitude(double latitude);
    double latitude() const;

    void setLongitude(double longitude);
    double longitude() const;

    void setAltitude(double altitude);
    double altitude() const;

    qreal distanceTo(const QGeoCoordinate &other) const;
    qreal azimuthTo(const QGeoCoordinate &other) const;

    QGeoCoordinate atDistanceAndAzimuth(qreal distance, qreal azimuth, qreal distanceUp = 0.0) const;

    QString toString(CoordinateFormat format = DegreesMinutesSecondsWithHemisphere) const;

private:
    QSharedDataPointer<QGeoCoordinatePrivate> d;

    friend class QGeoCoordinatePrivate;
};

Q_DECLARE_TYPEINFO(QGeoCoordinate, Q_MOVABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_POSITIONING_EXPORT QDebug operator<<(QDebug, const QGeoCoordinate &);
#endif

#ifndef QT_NO_DATASTREAM
Q_POSITIONING_EXPORT QDataStream &operator<<(QDataStream &stream, const QGeoCoordinate &coordinate);
Q_POSITIONING_EXPORT QDataStream &operator>>(QDataStream &stream, QGeoCoordinate &coordinate);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QGeoCoordinate)

#endif
