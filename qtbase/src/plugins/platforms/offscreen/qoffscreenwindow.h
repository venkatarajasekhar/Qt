/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QOFFSCREENWINDOW_H
#define QOFFSCREENWINDOW_H

#include <qpa/qplatformbackingstore.h>
#include <qpa/qplatformwindow.h>

#include <qhash.h>

QT_BEGIN_NAMESPACE

class QOffscreenWindow : public QPlatformWindow
{
public:
    QOffscreenWindow(QWindow *window);
    ~QOffscreenWindow();

    void setGeometry(const QRect &rect);
    void setWindowState(Qt::WindowState state);

    QMargins frameMargins() const;

    void setVisible(bool visible);
    void requestActivateWindow();

    WId winId() const;

    static QOffscreenWindow *windowForWinId(WId id);

private:
    void setFrameMarginsEnabled(bool enabled);
    void setGeometryImpl(const QRect &rect);

    QRect m_normalGeometry;
    QMargins m_margins;
    bool m_positionIncludesFrame;
    bool m_visible;
    bool m_pendingGeometryChangeOnShow;
    WId m_winId;

    static QHash<WId, QOffscreenWindow *> m_windowForWinIdHash;
};

QT_END_NAMESPACE

#endif
