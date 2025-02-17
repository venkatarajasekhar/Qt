/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtLocation module of the Qt Toolkit.
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

#ifndef QGEOROUTEREQUEST_H
#define QGEOROUTEREQUEST_H

#include <QList>
#include <QExplicitlySharedDataPointer>
#include <QDateTime>

#include <QtLocation/qlocationglobal.h>
#include <QtPositioning/qgeocoordinate.h>

QT_BEGIN_NAMESPACE

class QGeoRectangle;
class QGeoRouteRequestPrivate;

class Q_LOCATION_EXPORT QGeoRouteRequest
{
public:
    enum TravelMode {
        CarTravel = 0x0001,
        PedestrianTravel = 0x0002,
        BicycleTravel = 0x0004,
        PublicTransitTravel = 0x0008,
        TruckTravel = 0x0010
    };
    Q_DECLARE_FLAGS(TravelModes, TravelMode)

    enum FeatureType {
        NoFeature = 0x00000000,
        TollFeature = 0x00000001,
        HighwayFeature = 0x00000002,
        PublicTransitFeature = 0x00000004,
        FerryFeature = 0x00000008,
        TunnelFeature = 0x00000010,
        DirtRoadFeature = 0x00000020,
        ParksFeature = 0x00000040,
        MotorPoolLaneFeature = 0x00000080
    };
    Q_DECLARE_FLAGS(FeatureTypes, FeatureType)

    enum FeatureWeight {
        NeutralFeatureWeight = 0x00000000,
        PreferFeatureWeight = 0x00000001,
        RequireFeatureWeight = 0x00000002,
        AvoidFeatureWeight = 0x00000004,
        DisallowFeatureWeight = 0x00000008
    };
    Q_DECLARE_FLAGS(FeatureWeights, FeatureWeight)

    enum RouteOptimization {
        ShortestRoute = 0x0001,
        FastestRoute = 0x0002,
        MostEconomicRoute = 0x0004,
        MostScenicRoute = 0x0008
    };
    Q_DECLARE_FLAGS(RouteOptimizations, RouteOptimization)

    enum SegmentDetail {
        NoSegmentData = 0x0000,
        BasicSegmentData = 0x0001
    };
    Q_DECLARE_FLAGS(SegmentDetails, SegmentDetail)

    enum ManeuverDetail {
        NoManeuvers = 0x0000,
        BasicManeuvers = 0x0001
    };
    Q_DECLARE_FLAGS(ManeuverDetails, ManeuverDetail)

    explicit QGeoRouteRequest(const QList<QGeoCoordinate> &waypoints = QList<QGeoCoordinate>());
    QGeoRouteRequest(const QGeoCoordinate &origin,
                     const QGeoCoordinate &destination);
    QGeoRouteRequest(const QGeoRouteRequest &other);

    ~QGeoRouteRequest();

    QGeoRouteRequest &operator= (const QGeoRouteRequest &other);

    bool operator == (const QGeoRouteRequest &other) const;
    bool operator != (const QGeoRouteRequest &other) const;

    void setWaypoints(const QList<QGeoCoordinate> &waypoints);
    QList<QGeoCoordinate> waypoints() const;

    void setExcludeAreas(const QList<QGeoRectangle> &areas);
    QList<QGeoRectangle> excludeAreas() const;

    // defaults to 0
    void setNumberAlternativeRoutes(int alternatives);
    int numberAlternativeRoutes() const;

    // defaults to TravelByCar
    void setTravelModes(TravelModes travelModes);
    TravelModes travelModes() const;

    void setFeatureWeight(FeatureType featureType, FeatureWeight featureWeight);
    FeatureWeight featureWeight(FeatureType featureType) const;
    QList<FeatureType> featureTypes() const;

    // defaults to OptimizeFastest
    void setRouteOptimization(RouteOptimizations optimization);
    RouteOptimizations routeOptimization() const;

    // defaults to BasicSegmentData
    void setSegmentDetail(SegmentDetail segmentDetail);
    SegmentDetail segmentDetail() const;

    // defaults to BasicManeuvers
    void setManeuverDetail(ManeuverDetail maneuverDetail);
    ManeuverDetail maneuverDetail() const;

private:
    QExplicitlySharedDataPointer<QGeoRouteRequestPrivate> d_ptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QGeoRouteRequest::TravelModes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QGeoRouteRequest::FeatureTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QGeoRouteRequest::FeatureWeights)
Q_DECLARE_OPERATORS_FOR_FLAGS(QGeoRouteRequest::RouteOptimizations)
Q_DECLARE_OPERATORS_FOR_FLAGS(QGeoRouteRequest::SegmentDetails)
Q_DECLARE_OPERATORS_FOR_FLAGS(QGeoRouteRequest::ManeuverDetails)

QT_END_NAMESPACE

#endif
