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
#ifndef QGEOCAMERATILES_P_H
#define QGEOCAMERATILES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtLocation/qlocationglobal.h>
#include <QSet>
#include <QSize>
#include <QSizeF>
#include <QPair>

QT_BEGIN_NAMESPACE

class QGeoCameraData;
class QGeoTileSpec;
class QGeoMapType;

class QGeoCameraTilesPrivate;

class Q_LOCATION_EXPORT QGeoCameraTiles {
public:
    QGeoCameraTiles();
    ~QGeoCameraTiles();

    void setCamera(const QGeoCameraData &camera);
    void setScreenSize(const QSize &size);
    void setTileSize(int tileSize);
    void setMaximumZoomLevel(int maxZoom);

    int tileSize() const;

    void setPluginString(const QString &pluginString);
    void setMapType(const QGeoMapType &mapType);
    void setMapVersion(int mapVersion);

    QSet<QGeoTileSpec> tiles() const;
    void findPrefetchTiles();

private:
    QGeoCameraTilesPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QGeoCameraTiles)
    Q_DISABLE_COPY(QGeoCameraTiles)
};

typedef struct 
{
    QDoubleVector3D topLeftNear;
    QDoubleVector3D topLeftFar;
    QDoubleVector3D topRightNear;
    QDoubleVector3D topRightFar;
    QDoubleVector3D bottomLeftNear;
    QDoubleVector3D bottomLeftFar;
    QDoubleVector3D bottomRightNear;
    QDoubleVector3D bottomRightFar;
}Frustum_t;

typedef QVector<QDoubleVector3D> PolygonVector;

class QGeoCameraTilesPrivate
{
public:
    QGeoCameraTilesPrivate();
    ~QGeoCameraTilesPrivate();

    QString pluginString_;
    QGeoMapType mapType_;
    int mapVersion_;
    QGeoCameraData camera_;
    QSize screenSize_;
    int tileSize_;
    int maxZoom_;
    QSet<QGeoTileSpec> tiles_;

    int intZoomLevel_;
    int sideLength_;

    void updateMetadata();
    void updateGeometry(double viewExpansion = 1.0);

    Frustum frustum(double fieldOfViewGradient) const;

    class LengthSorter
    {
    public:
        QDoubleVector3D base;
        bool operator()(const QDoubleVector3D &lhs, const QDoubleVector3D &rhs)
        {
            return (lhs - base).lengthSquared() < (rhs - base).lengthSquared();
        }
    };

    void appendZIntersects(const QDoubleVector3D &start, const QDoubleVector3D &end, double z, QVector<QDoubleVector3D> &results) const;
    PolygonVector frustumFootprint(const Frustum &frustum) const;

    QPair<PolygonVector, PolygonVector> splitPolygonAtAxisValue(const PolygonVector &polygon, int axis, double value) const;
    QPair<PolygonVector, PolygonVector> clipFootprintToMap(const PolygonVector &footprint) const;

    QList<QPair<double, int> > tileIntersections(double p1, int t1, double p2, int t2) const;
    QSet<QGeoTileSpec> tilesFromPolygon(const PolygonVector &polygon) const;

    typedef struct 
    {
        TileMap();

        void add(int tileX, int tileY);

        QMap<int, QPair<int, int> > data;
    }TileMap_t;
};

QT_END_NAMESPACE

#endif // QGEOCAMERATILES_P_H
