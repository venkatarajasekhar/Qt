/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include "qwindowdump.h"

#if QT_VERSION > 0x050000
#  include <QtGui/QGuiApplication>
#  include <QtGui/QScreen>
#  include <QtGui/QWindow>
#  include <qpa/qplatformwindow.h>
#endif
#include <QtCore/QMetaObject>
#include <QtCore/QRect>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>

namespace QtDiag {

void indentStream(QTextStream &s, int indent)
{
    for (int i = 0; i < indent; ++i)
        s << ' ';
}

void formatObject(QTextStream &str, const QObject *o)
{
    str << o->metaObject()->className();
    const QString on = o->objectName();
    if (!on.isEmpty())
        str << "/\"" << on << '"';
}

void formatRect(QTextStream &str, const QRect &geom)
{
    str << geom.width() << 'x' << geom.height()
        << forcesign << geom.x() << geom.y() << noforcesign;
}

#define debugType(s, type, typeConstant) \
if ((type & typeConstant) == typeConstant) \
    s << ' ' << #typeConstant;

#define debugFlag(s, flags, flagConstant) \
if (flags & flagConstant) \
    s << ' ' << #flagConstant;

void formatWindowFlags(QTextStream &str, const Qt::WindowFlags flags)
{
    str << showbase << hex << unsigned(flags) << dec << noshowbase;
    const Qt::WindowFlags windowType = flags & Qt::WindowType_Mask;
    debugFlag(str, flags, Qt::Window)
    debugType(str, windowType, Qt::Dialog)
    debugType(str, windowType, Qt::Sheet)
    debugType(str, windowType, Qt::Drawer)
    debugType(str, windowType, Qt::Popup)
    debugType(str, windowType, Qt::Tool)
    debugType(str, windowType, Qt::ToolTip)
    debugType(str, windowType, Qt::SplashScreen)
    debugType(str, windowType, Qt::Desktop)
    debugType(str, windowType, Qt::SubWindow)
#if QT_VERSION > 0x050000
    debugType(str, windowType, Qt::ForeignWindow)
    debugType(str, windowType, Qt::CoverWindow)
#endif
    debugFlag(str, flags, Qt::MSWindowsFixedSizeDialogHint)
    debugFlag(str, flags, Qt::MSWindowsOwnDC)
    debugFlag(str, flags, Qt::X11BypassWindowManagerHint)
    debugFlag(str, flags, Qt::FramelessWindowHint)
    debugFlag(str, flags, Qt::WindowTitleHint)
    debugFlag(str, flags, Qt::WindowSystemMenuHint)
    debugFlag(str, flags, Qt::WindowMinimizeButtonHint)
    debugFlag(str, flags, Qt::WindowMaximizeButtonHint)
    debugFlag(str, flags, Qt::WindowContextHelpButtonHint)
    debugFlag(str, flags, Qt::WindowShadeButtonHint)
    debugFlag(str, flags, Qt::WindowStaysOnTopHint)
    debugFlag(str, flags, Qt::CustomizeWindowHint)
#if QT_VERSION > 0x050000
    debugFlag(str, flags, Qt::WindowTransparentForInput)
    debugFlag(str, flags, Qt::WindowOverridesSystemGestures)
    debugFlag(str, flags, Qt::WindowDoesNotAcceptFocus)
    debugFlag(str, flags, Qt::NoDropShadowWindowHint)
    debugFlag(str, flags, Qt::WindowFullscreenButtonHint)
#endif
    debugFlag(str, flags, Qt::WindowStaysOnBottomHint)
    debugFlag(str, flags, Qt::MacWindowToolBarButtonHint)
    debugFlag(str, flags, Qt::BypassGraphicsProxyWidget)
    debugFlag(str, flags, Qt::WindowOkButtonHint)
    debugFlag(str, flags, Qt::WindowCancelButtonHint)
}

#if QT_VERSION > 0x050000

void formatWindow(QTextStream &str, const QWindow *w, FormatWindowOptions options)
{
    const QPlatformWindow *pw = w->handle();
    formatObject(str, w);
    str << ' ' << (w->isVisible() ? "[visible] " : "[hidden] ");
    if (const WId nativeWinId = pw ? pw->winId() : WId(0))
        str << "[native: " << hex << showbase << nativeWinId << dec << noshowbase << "] ";
    if (w->isTopLevel())
        str << "[top] ";
    if (w->isExposed())
        str << "[exposed] ";
    formatRect(str, w->geometry());
    if (!(options & DontPrintWindowFlags)) {
        str << ' ';
        formatWindowFlags(str, w->flags());
    }
    str << '\n';
}

static void dumpWindowRecursion(QTextStream &str, const QWindow *w,
                                FormatWindowOptions options = 0, int depth = 0)
{
    indentStream(str, 2 * depth);
    formatWindow(str, w);
    foreach (const QObject *co, w->children()) {
        if (co->isWindowType())
            dumpWindowRecursion(str, static_cast<const QWindow *>(co), options, depth + 1);
    }
}

void dumpAllWindows(FormatWindowOptions options)
{
    QString d;
    QTextStream str(&d);
    str << "### QWindows:\n";
    foreach (QWindow *w, QGuiApplication::topLevelWindows())
        dumpWindowRecursion(str, w, options);
    qDebug().noquote() << d;
}

#else // Qt 5
class QWindow {};

void formatWindow(QTextStream &, const QWindow *, FormatWindowOptions)
{
}

void dumpAllWindows(FormatWindowOptions options)
{
}

#endif // Qt 4

} // namespace QtDiag
