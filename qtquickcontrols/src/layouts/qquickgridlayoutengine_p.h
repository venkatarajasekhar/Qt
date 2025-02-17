/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Quick Layouts module of the Qt Toolkit.
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

#ifndef QQUICKGRIDLAYOUTENGINE_P_H
#define QQUICKGRIDLAYOUTENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the graphics view layout classes.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qgridlayoutengine_p.h>
#include <QtGui/private/qlayoutpolicy_p.h>
#include <QtCore/qmath.h>

#include "qquickitem.h"
#include "qquicklayout_p.h"
#include "qdebug.h"
QT_BEGIN_NAMESPACE

class QQuickGridLayoutItem : public QGridLayoutItem {
public:
    QQuickGridLayoutItem(QQuickItem *item, int row, int column,
                         int rowSpan = 1, int columnSpan = 1, Qt::Alignment alignment = 0)
        : QGridLayoutItem(row, column, rowSpan, columnSpan, alignment), m_item(item), sizeHintCacheDirty(true), useFallbackToWidthOrHeight(true) {}


    typedef qreal (QQuickLayoutAttached::*SizeGetter)() const;

    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
    {
        Q_UNUSED(constraint);   // Quick Layouts does not support constraint atm
        return effectiveSizeHints()[which];
    }

    static void effectiveSizeHints_helper(QQuickItem *item, QSizeF *cachedSizeHints, QQuickLayoutAttached **info, bool useFallbackToWidthOrHeight);
    static QLayoutPolicy::Policy effectiveSizePolicy_helper(QQuickItem *item, Qt::Orientation orientation, QQuickLayoutAttached *info);

    QSizeF *effectiveSizeHints() const
    {
        if (!sizeHintCacheDirty)
            return cachedSizeHints;

        effectiveSizeHints_helper(m_item, cachedSizeHints, 0, useFallbackToWidthOrHeight);
        useFallbackToWidthOrHeight = false;

        sizeHintCacheDirty = false;
        return cachedSizeHints;
    }

    void setCachedSizeHints(QSizeF *sizeHints)
    {
        for (int i = 0; i < Qt::NSizeHints; ++i) {
            cachedSizeHints[i] = sizeHints[i];
        }
        sizeHintCacheDirty = false;
    }

    void invalidate()
    {
        quickLayoutDebug() << "engine::invalidate()";
        sizeHintCacheDirty = true;
    }

    QLayoutPolicy::Policy sizePolicy(Qt::Orientation orientation) const
    {
        return effectiveSizePolicy_helper(m_item, orientation, attachedLayoutObject(m_item, false));
    }

    void setGeometry(const QRectF &rect)
    {
        const QSizeF oldSize(m_item->width(), m_item->height());
        const QSizeF newSize = rect.size();
        const QPointF topLeft(qCeil(rect.x()), qCeil(rect.y()));
        m_item->setPosition(topLeft);
        if (newSize == oldSize) {
            if (QQuickLayout *lay = qobject_cast<QQuickLayout *>(m_item)) {
                if (lay->arrangementIsDirty())
                    lay->rearrange(newSize);
            }
        } else {
            m_item->setSize(newSize);
        }
    }

    QQuickItem *layoutItem() const { return m_item; }

    QQuickItem *m_item;
private:
    mutable QSizeF cachedSizeHints[Qt::NSizeHints];
    mutable unsigned sizeHintCacheDirty : 1;
    mutable unsigned useFallbackToWidthOrHeight : 1;
};

class QQuickGridLayoutEngine : public QGridLayoutEngine {
public:
    QQuickGridLayoutEngine() : QGridLayoutEngine(Qt::AlignVCenter) { }

    int indexOf(QQuickItem *item) const {
        for (int i = 0; i < q_items.size(); ++i) {
            if (item == static_cast<QQuickGridLayoutItem*>(q_items.at(i))->layoutItem())
                return i;
        }
        return -1;
    }

    QQuickGridLayoutItem *findLayoutItem(QQuickItem *layoutItem) const
    {
        for (int i = q_items.count() - 1; i >= 0; --i) {
            QQuickGridLayoutItem *item = static_cast<QQuickGridLayoutItem*>(q_items.at(i));
            if (item->layoutItem() == layoutItem)
                return item;
        }
        return 0;
    }

    void setAlignment(QQuickItem *quickItem, Qt::Alignment alignment);
    Qt::Alignment alignment(QQuickItem *quickItem) const;

};



QT_END_NAMESPACE

#endif // QQUICKGRIDLAYOUTENGINE_P_H
