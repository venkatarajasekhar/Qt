/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#ifndef RUBBERBANDSELECTIONMANIPULATOR_H
#define RUBBERBANDSELECTIONMANIPULATOR_H

#include "liveselectionrectangle.h"

#include <QtCore/QPointF>

QT_FORWARD_DECLARE_CLASS(QGraphicsItem)

namespace QmlJSDebugger {
namespace QtQuick1 {

class QDeclarativeViewInspector;

class LiveRubberBandSelectionManipulator
{
public:
    enum SelectionType {
        ReplaceSelection,
        AddToSelection,
        RemoveFromSelection
    };

    LiveRubberBandSelectionManipulator(QGraphicsObject *layerItem,
                                       QDeclarativeViewInspector *editorView);

    void setItems(const QList<QGraphicsItem*> &itemList);

    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint);
    void end();

    void clear();

    void select(SelectionType selectionType);

    QPointF beginPoint() const;

    bool isActive() const;

protected:
    QGraphicsItem *topFormEditorItem(const QList<QGraphicsItem*> &itemList);

private:
    QList<QGraphicsItem*> m_itemList;
    QList<QGraphicsItem*> m_oldSelectionList;
    LiveSelectionRectangle m_selectionRectangleElement;
    QPointF m_beginPoint;
    QDeclarativeViewInspector *m_editorView;
    QGraphicsItem *m_beginFormEditorItem;
    bool m_isActive;
};

} // namespace QtQuick1
} // namespace QmlJSDebugger

#endif // RUBBERBANDSELECTIONMANIPULATOR_H
