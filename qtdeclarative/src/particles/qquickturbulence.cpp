/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickturbulence_p.h"
#include "qquickparticlepainter_p.h"//TODO: Why was this needed again?
#include <cmath>
#include <cstdlib>
#include <QDebug>
QT_BEGIN_NAMESPACE

/*!
    \qmltype Turbulence
    \instantiates QQuickTurbulenceAffector
    \inqmlmodule QtQuick.Particles
    \ingroup qtquick-particles
    \inherits Affector
    \brief Provides fluid-like forces from a noise image

    The Turbulence Element scales the noise source over the area it affects,
    and uses the curl of that source to generate force vectors.

    Turbulence requires a fixed size. Unlike other affectors, a 0x0 Turbulence element
    will affect no particles.

    The source should be relatively smooth black and white noise, such as perlin noise.
*/
/*!
    \qmlproperty real QtQuick.Particles::Turbulence::strength

    The magnitude of the velocity vector at any point varies between zero and
    the square root of two. It will then be multiplied by strength to get the
    velocity per second for the particles affected by the turbulence.
*/
/*!
    \qmlproperty url QtQuick.Particles::Turbulence::noiseSource

    The source image to generate the turbulence from. It will be scaled to the size of the element,
    so equal or larger sizes will give better results. Tweaking this image is the only way to tweak
    behavior such as where vortices are or how many exist.

    The source should be a relatively smooth black and white noise image, such as perlin noise.
    A default image will be used if none is provided.
*/

QQuickTurbulenceAffector::QQuickTurbulenceAffector(QQuickItem *parent) :
    QQuickParticleAffector(parent),
    m_strength(10), m_lastT(0), m_gridSize(0), m_field(0), m_vectorField(0), m_inited(false)
{
}

void QQuickTurbulenceAffector::geometryChanged(const QRectF &, const QRectF &)
{
    initializeGrid();
}

QQuickTurbulenceAffector::~QQuickTurbulenceAffector()
{
    if (m_field) {
        for (int i=0; i<m_gridSize; i++)
            free(m_field[i]);
        free(m_field);
    }
    if (m_vectorField) {
        for (int i=0; i<m_gridSize; i++)
            free(m_vectorField[i]);
        free(m_vectorField);
    }
}

void QQuickTurbulenceAffector::initializeGrid()
{
    if (!m_inited)
        return;

    int arg = qMax(width(), height());
    if (m_gridSize != arg) {
        if (m_field){ //deallocate and then reallocate grid
            for (int i=0; i<m_gridSize; i++)
                free(m_field[i]);
            free(m_field);
            m_system = 0;
        }
        if (m_vectorField) {
            for (int i=0; i<m_gridSize; i++)
                free(m_vectorField[i]);
            free(m_vectorField);
        }
        m_gridSize = arg;
    }

    m_field = (qreal**)malloc(m_gridSize * sizeof(qreal*));
    for (int i=0; i<m_gridSize; i++)
        m_field[i] = (qreal*)malloc(m_gridSize * sizeof(qreal));
    m_vectorField = (QPointF**)malloc(m_gridSize * sizeof(QPointF*));
    for (int i=0; i<m_gridSize; i++)
        m_vectorField[i] = (QPointF*)malloc(m_gridSize * sizeof(QPointF));

    QImage image;
    if (!m_noiseSource.isEmpty())
        image = QImage(m_noiseSource.toLocalFile()).scaled(QSize(m_gridSize, m_gridSize));
    if (image.isNull())
        image = QImage(QStringLiteral(":particleresources/noise.png")).scaled(QSize(m_gridSize, m_gridSize));

    for (int i=0; i<m_gridSize; i++)
        for (int j=0; j<m_gridSize; j++)
            m_field[i][j] = qRed(image.pixel(QPoint(i,j)));//Red as proxy for Value
    for (int i=0; i<m_gridSize; i++){
        for (int j=0; j<m_gridSize; j++){
            m_vectorField[i][j].setX(boundsRespectingField(i,j) - boundsRespectingField(i,j-1));
            m_vectorField[i][j].setY(boundsRespectingField(i-1,j) - boundsRespectingField(i,j));
        }
    }
}

qreal QQuickTurbulenceAffector::boundsRespectingField(int x, int y)
{
    if (x < 0)
        x = 0;
    if (x >= m_gridSize)
        x = m_gridSize - 1;
    if (y < 0)
        y = 0;
    if (y >= m_gridSize)
        y = m_gridSize - 1;
    return m_field[x][y];
}

void QQuickTurbulenceAffector::ensureInit()
{
    if (m_inited)
        return;
    m_inited = true;
    initializeGrid();
}

void QQuickTurbulenceAffector::affectSystem(qreal dt)
{
    if (!m_system || !m_enabled)
        return;
    ensureInit();
    if (!m_gridSize)
        return;

    updateOffsets();//### Needed if an ancestor is transformed.

    QRect boundsRect(0,0,m_gridSize,m_gridSize);
    foreach (QQuickParticleGroupData *gd, m_system->groupData){
        if (!activeGroup(m_system->groupData.key(gd)))
            continue;
        foreach (QQuickParticleData *d, gd->data){
            if (!shouldAffect(d))
                continue;
            QPoint pos = (QPointF(d->curX(), d->curY()) - m_offset).toPoint();
            if (!boundsRect.contains(pos,true))//Need to redo bounds checking due to quantization.
                continue;
            qreal fx = 0.0;
            qreal fy = 0.0;
            fx += m_vectorField[pos.x()][pos.y()].x() * m_strength;
            fy += m_vectorField[pos.x()][pos.y()].y() * m_strength;
            if (fx || fy){
                d->setInstantaneousVX(d->curVX()+ fx * dt);
                d->setInstantaneousVY(d->curVY()+ fy * dt);
                postAffect(d);
            }
        }
    }
}

QT_END_NAMESPACE
