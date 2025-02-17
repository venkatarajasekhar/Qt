/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Aaron McCarthy <aaron.mccarthy@jollamobile.com>
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
***************************************************************************/

#ifndef QDECLARATIVEPOSITION_H
#define QDECLARATIVEPOSITION_H

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtQml/qqml.h>
#include <QtPositioning/QGeoPositionInfo>

QT_BEGIN_NAMESPACE

class QDeclarativePosition : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool latitudeValid READ isLatitudeValid NOTIFY latitudeValidChanged)
    Q_PROPERTY(bool longitudeValid READ isLongitudeValid NOTIFY longitudeValidChanged)
    Q_PROPERTY(bool altitudeValid READ isAltitudeValid NOTIFY altitudeValidChanged)
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate NOTIFY coordinateChanged)
    Q_PROPERTY(QDateTime timestamp READ timestamp NOTIFY timestampChanged)
    Q_PROPERTY(double speed READ speed NOTIFY speedChanged)
    Q_PROPERTY(bool speedValid READ isSpeedValid NOTIFY speedValidChanged)
    Q_PROPERTY(qreal horizontalAccuracy READ horizontalAccuracy WRITE setHorizontalAccuracy NOTIFY horizontalAccuracyChanged)
    Q_PROPERTY(qreal verticalAccuracy READ verticalAccuracy WRITE setVerticalAccuracy NOTIFY verticalAccuracyChanged)
    Q_PROPERTY(bool horizontalAccuracyValid READ isHorizontalAccuracyValid NOTIFY horizontalAccuracyValidChanged)
    Q_PROPERTY(bool verticalAccuracyValid READ isVerticalAccuracyValid NOTIFY verticalAccuracyValidChanged)

    Q_PROPERTY(bool directionValid READ isDirectionValid NOTIFY directionValidChanged REVISION 1)
    Q_PROPERTY(double direction READ direction NOTIFY directionChanged REVISION 1)
    Q_PROPERTY(bool verticalSpeedValid READ isVerticalSpeedValid NOTIFY verticalSpeedValidChanged REVISION 1)
    Q_PROPERTY(double verticalSpeed READ verticalSpeed NOTIFY verticalSpeedChanged REVISION 1)

    Q_PROPERTY(double magneticVariation READ magneticVariation NOTIFY magneticVariationChanged REVISION 2)
    Q_PROPERTY(bool magneticVariationValid READ isMagneticVariationValid NOTIFY magneticVariationChanged REVISION 2)

public:
    explicit QDeclarativePosition(QObject *parent = 0);
    ~QDeclarativePosition();

    bool isLatitudeValid() const;
    bool isLongitudeValid() const;
    bool isAltitudeValid() const;
    QDateTime timestamp() const;
    double speed() const;
    bool isSpeedValid() const;
    QGeoCoordinate coordinate();
    bool isHorizontalAccuracyValid() const;
    qreal horizontalAccuracy() const;
    void setHorizontalAccuracy(qreal horizontalAccuracy);
    bool isVerticalAccuracyValid() const;
    qreal verticalAccuracy() const;
    void setVerticalAccuracy(qreal verticalAccuracy);

    bool isDirectionValid() const;
    double direction() const;
    void setDirection(double direction);

    bool isVerticalSpeedValid() const;
    double verticalSpeed() const;
    void setVerticalSpeed(double speed);

    bool isMagneticVariationValid() const;
    double magneticVariation() const;

    void setPosition(const QGeoPositionInfo &info);

Q_SIGNALS:
    void latitudeValidChanged();
    void longitudeValidChanged();
    void altitudeValidChanged();
    void timestampChanged();
    void speedChanged();
    void speedValidChanged();
    void coordinateChanged();
    void horizontalAccuracyChanged();
    void horizontalAccuracyValidChanged();
    void verticalAccuracyChanged();
    void verticalAccuracyValidChanged();

    Q_REVISION(1) void directionValidChanged();
    Q_REVISION(1) void directionChanged();
    Q_REVISION(1) void verticalSpeedValidChanged();
    Q_REVISION(1) void verticalSpeedChanged();

    Q_REVISION(2) void magneticVariationChanged();
    Q_REVISION(2) void magneticVariationValidChanged();

private:
    QGeoPositionInfo m_info;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativePosition)

#endif
