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

#include "nativewindowdump.h"
#include "qwindowdump.h"

#include <QtCore/QTextStream>
#include <QtCore/QSharedPointer>
#include <QtCore/QDebug>
#include <QtCore/QVector>

#include <QtCore/qt_windows.h>

namespace QtDiag {

struct DumpContext {
    DumpContext() : indentation(0) {}

    int indentation;
    QSharedPointer<QTextStream> stream;
};

#define debugWinStyle(str, style, styleConstant) \
if (style & styleConstant) \
    str << ' ' << #styleConstant;

static void formatNativeWindow(HWND hwnd, QTextStream &str)
{
    str << hex << showbase << quintptr(hwnd) << noshowbase << dec;
    RECT rect;
    if (GetWindowRect(hwnd, &rect)) {
        str << ' ' << (rect.right - rect.left) << 'x' << (rect.bottom - rect.top)
            << '+' << rect.left << '+' << rect.top;
    }
    if (IsWindowVisible(hwnd))
        str << " [visible]";

    str << hex << showbase;
    if (const LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE)) {
        str << " style=" << style;
        debugWinStyle(str, style, WS_OVERLAPPED)
        debugWinStyle(str, style, WS_POPUP)
        debugWinStyle(str, style, WS_MINIMIZE)
        debugWinStyle(str, style, WS_CHILD)
        debugWinStyle(str, style, WS_VISIBLE)
        debugWinStyle(str, style, WS_DISABLED)
        debugWinStyle(str, style, WS_CLIPSIBLINGS)
        debugWinStyle(str, style, WS_CLIPCHILDREN)
        debugWinStyle(str, style, WS_MAXIMIZE)
        debugWinStyle(str, style, WS_CAPTION)
        debugWinStyle(str, style, WS_BORDER)
        debugWinStyle(str, style, WS_DLGFRAME)
        debugWinStyle(str, style, WS_VSCROLL)
        debugWinStyle(str, style, WS_HSCROLL)
        debugWinStyle(str, style, WS_SYSMENU)
        debugWinStyle(str, style, WS_THICKFRAME)
        debugWinStyle(str, style, WS_GROUP)
        debugWinStyle(str, style, WS_TABSTOP)
        debugWinStyle(str, style, WS_MINIMIZEBOX)
        debugWinStyle(str, style, WS_MAXIMIZEBOX)
    }
    if (const LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE)) {
        str << " exStyle=" << exStyle;
        debugWinStyle(str, exStyle, WS_EX_DLGMODALFRAME)
        debugWinStyle(str, exStyle, WS_EX_NOPARENTNOTIFY)
        debugWinStyle(str, exStyle, WS_EX_TOPMOST)
        debugWinStyle(str, exStyle, WS_EX_ACCEPTFILES)
        debugWinStyle(str, exStyle, WS_EX_TRANSPARENT)
        debugWinStyle(str, exStyle, WS_EX_MDICHILD)
        debugWinStyle(str, exStyle, WS_EX_TOOLWINDOW)
        debugWinStyle(str, exStyle, WS_EX_WINDOWEDGE)
        debugWinStyle(str, exStyle, WS_EX_CLIENTEDGE)
        debugWinStyle(str, exStyle, WS_EX_CONTEXTHELP)
        debugWinStyle(str, exStyle, WS_EX_RIGHT)
        debugWinStyle(str, exStyle, WS_EX_LEFT)
        debugWinStyle(str, exStyle, WS_EX_RTLREADING)
        debugWinStyle(str, exStyle, WS_EX_LTRREADING)
        debugWinStyle(str, exStyle, WS_EX_LEFTSCROLLBAR)
        debugWinStyle(str, exStyle, WS_EX_RIGHTSCROLLBAR)
        debugWinStyle(str, exStyle, WS_EX_CONTROLPARENT)
        debugWinStyle(str, exStyle, WS_EX_STATICEDGE)
        debugWinStyle(str, exStyle, WS_EX_APPWINDOW)
        debugWinStyle(str, exStyle, WS_EX_LAYERED)
        debugWinStyle(str, exStyle, WS_EX_NOINHERITLAYOUT)
        debugWinStyle(str, exStyle, WS_EX_NOREDIRECTIONBITMAP)
        debugWinStyle(str, exStyle, WS_EX_LAYOUTRTL)
        debugWinStyle(str, exStyle, WS_EX_COMPOSITED)
        debugWinStyle(str, exStyle, WS_EX_NOACTIVATE)
    }
    str << noshowbase << dec;

    wchar_t buf[512];
    if (GetWindowText(hwnd, buf, sizeof(buf)/sizeof(buf[0])))
        str << " title=\"" << QString::fromWCharArray(buf) << '"';
    if (GetClassName(hwnd, buf, sizeof(buf)/sizeof(buf[0])))
        str << " class=\"" << QString::fromWCharArray(buf) << '"';
    str << '\n';
}

static void dumpNativeWindowRecursion(HWND hwnd, DumpContext *dc);

BOOL CALLBACK dumpWindowEnumChildProc(HWND hwnd, LPARAM lParam)
{
    dumpNativeWindowRecursion(hwnd, reinterpret_cast<DumpContext *>(lParam));
    return TRUE;
}

static void dumpNativeWindowRecursion(HWND hwnd, DumpContext *dc)
{
    indentStream(*dc->stream, dc->indentation);
    formatNativeWindow(hwnd, *dc->stream);
    DumpContext nextLevel = *dc;
    nextLevel.indentation += 2;
    EnumChildWindows(hwnd, dumpWindowEnumChildProc, reinterpret_cast<LPARAM>(&nextLevel));
}

/* recurse by Z order, same result */
static void dumpNativeWindowRecursionByZ(HWND hwnd, DumpContext *dc)
{
    indentStream(*dc->stream, dc->indentation);
    formatNativeWindow(hwnd, *dc->stream);
    if (const HWND topChild = GetTopWindow(hwnd)) {
        DumpContext nextLevel = *dc;
        nextLevel.indentation += 2;
        for (HWND child = topChild; child ; child = GetNextWindow(child, GW_HWNDNEXT))
            dumpNativeWindowRecursionByZ(child, &nextLevel);
    }
}

typedef QVector<WId> WIdVector;

static void dumpNativeWindows(const WIdVector& wins)
{
    DumpContext dc;
    QString s;
    dc.stream = QSharedPointer<QTextStream>(new QTextStream(&s));
    foreach (WId win, wins)
        dumpNativeWindowRecursion(reinterpret_cast<HWND>(win), &dc);
#if QT_VERSION > 0x050000
    qDebug().noquote() << s;
#else
    qDebug("%s", qPrintable(s));
#endif
}

void dumpNativeWindows(WId rootIn)
{
    const WId root = rootIn ? rootIn : reinterpret_cast<WId>(GetDesktopWindow());
    dumpNativeWindows(WIdVector(1, root));
}

BOOL CALLBACK findQtTopLevelEnumChildProc(HWND hwnd, LPARAM lParam)
{
    WIdVector *v = reinterpret_cast<WIdVector *>(lParam);
    wchar_t buf[512];
    if (GetClassName(hwnd, buf, sizeof(buf)/sizeof(buf[0]))) {
        if (wcsstr(buf, L"Qt"))
            v->append(reinterpret_cast<WId>(hwnd));
    }
    return TRUE;
}

static WIdVector findQtTopLevels()
{
    WIdVector result;
    EnumChildWindows(GetDesktopWindow(),
                     findQtTopLevelEnumChildProc,
                     reinterpret_cast<LPARAM>(&result));
    return result;
}

void dumpNativeQtTopLevels()
{
    dumpNativeWindows(findQtTopLevels());
}

} // namespace QtDiag
