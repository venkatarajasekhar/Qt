/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
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

#include "qsgvideonode_p.h"

QT_BEGIN_NAMESPACE

QSGVideoNode::QSGVideoNode()
    : m_orientation(-1)
{
    setFlag(QSGNode::OwnsGeometry);
}

/* Helpers */
static inline void qSetGeom(QSGGeometry::TexturedPoint2D *v, const QPointF &p)
{
    v->x = p.x();
    v->y = p.y();
}

static inline void qSetTex(QSGGeometry::TexturedPoint2D *v, const QPointF &p)
{
    v->tx = p.x();
    v->ty = p.y();
}

/* Update the vertices and texture coordinates.  Orientation must be in {0,90,180,270} */
void QSGVideoNode::setTexturedRectGeometry(const QRectF &rect, const QRectF &textureRect, int orientation)
{
    if (rect == m_rect && textureRect == m_textureRect && orientation == m_orientation)
        return;

    m_rect = rect;
    m_textureRect = textureRect;
    m_orientation = orientation;

    QSGGeometry *g = geometry();

    if (g == 0)
        g = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);

    QSGGeometry::TexturedPoint2D *v = g->vertexDataAsTexturedPoint2D();

    // Set geometry first
    qSetGeom(v + 0, rect.topLeft());
    qSetGeom(v + 1, rect.bottomLeft());
    qSetGeom(v + 2, rect.topRight());
    qSetGeom(v + 3, rect.bottomRight());

    // and then texture coordinates
    switch (orientation) {
        default:
            // tl, bl, tr, br
            qSetTex(v + 0, textureRect.topLeft());
            qSetTex(v + 1, textureRect.bottomLeft());
            qSetTex(v + 2, textureRect.topRight());
            qSetTex(v + 3, textureRect.bottomRight());
            break;

        case 90:
            // tr, tl, br, bl
            qSetTex(v + 0, textureRect.topRight());
            qSetTex(v + 1, textureRect.topLeft());
            qSetTex(v + 2, textureRect.bottomRight());
            qSetTex(v + 3, textureRect.bottomLeft());
            break;

        case 180:
            // br, tr, bl, tl
            qSetTex(v + 0, textureRect.bottomRight());
            qSetTex(v + 1, textureRect.topRight());
            qSetTex(v + 2, textureRect.bottomLeft());
            qSetTex(v + 3, textureRect.topLeft());
            break;

        case 270:
            // bl, br, tl, tr
            qSetTex(v + 0, textureRect.bottomLeft());
            qSetTex(v + 1, textureRect.bottomRight());
            qSetTex(v + 2, textureRect.topLeft());
            qSetTex(v + 3, textureRect.topRight());
            break;
    }

    if (!geometry())
        setGeometry(g);

    markDirty(DirtyGeometry);
}

QT_END_NAMESPACE
