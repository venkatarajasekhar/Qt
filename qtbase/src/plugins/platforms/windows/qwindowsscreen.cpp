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

#include "qwindowsscreen.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"
#include "qwindowsintegration.h"
#include "qwindowscursor.h"

#include "qtwindows_additional.h"

#include <QtGui/QPixmap>
#include <QtGui/QGuiApplication>
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/QScreen>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

QWindowsScreenData::QWindowsScreenData() :
    dpi(96, 96), depth(32), format(QImage::Format_ARGB32_Premultiplied),
    flags(VirtualDesktop), orientation(Qt::LandscapeOrientation),
    refreshRateHz(60)
{
}

static inline QDpi deviceDPI(HDC hdc)
{
    return QDpi(GetDeviceCaps(hdc, LOGPIXELSX), GetDeviceCaps(hdc, LOGPIXELSY));
}

#ifndef Q_OS_WINCE

static inline QDpi monitorDPI(HMONITOR hMonitor)
{
    if (QWindowsContext::shcoredll.isValid()) {
        UINT dpiX;
        UINT dpiY;
        if (SUCCEEDED(QWindowsContext::shcoredll.getDpiForMonitor(hMonitor, 0, &dpiX, &dpiY)))
            return QDpi(dpiX, dpiY);
    }
    return QDpi(0, 0);
}

#endif // !Q_OS_WINCE

static inline QSizeF deviceSizeMM(const QSize &pixels, const QDpi &dpi)
{
    const qreal inchToMM = 25.4;
    const qreal h = qreal(pixels.width())  / qreal(dpi.first)  * inchToMM;
    const qreal v = qreal(pixels.height()) / qreal(dpi.second) * inchToMM;
    return QSizeF(h, v);
}

static inline QDpi deviceDPI(const QSize &pixels, const QSizeF &physicalSizeMM)
{
    const qreal inchToMM = 25.4;
    const qreal h = qreal(pixels.width())  / (qreal(physicalSizeMM.width())  / inchToMM);
    const qreal v = qreal(pixels.height()) / (qreal(physicalSizeMM.height()) / inchToMM);
    return QDpi(h, v);
}

typedef QList<QWindowsScreenData> WindowsScreenDataList;

static bool monitorData(HMONITOR hMonitor, QWindowsScreenData *data)
{
    MONITORINFOEX info;
    memset(&info, 0, sizeof(MONITORINFOEX));
    info.cbSize = sizeof(MONITORINFOEX);
    if (GetMonitorInfo(hMonitor, &info) == FALSE)
        return false;

    data->geometry = QRect(QPoint(info.rcMonitor.left, info.rcMonitor.top), QPoint(info.rcMonitor.right - 1, info.rcMonitor.bottom - 1));
    data->availableGeometry = QRect(QPoint(info.rcWork.left, info.rcWork.top), QPoint(info.rcWork.right - 1, info.rcWork.bottom - 1));
    data->name = QString::fromWCharArray(info.szDevice);
    if (data->name == QLatin1String("WinDisc")) {
        data->flags |= QWindowsScreenData::LockScreen;
    } else {
#ifdef Q_OS_WINCE
        //Windows CE, just supports one Display and expects to get only DISPLAY,
        //instead of DISPLAY0 and so on, which are passed by info.szDevice
        HDC hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
#else
        HDC hdc = CreateDC(info.szDevice, NULL, NULL, NULL);
#endif
        if (hdc) {
#ifndef Q_OS_WINCE
            const QDpi dpi = monitorDPI(hMonitor);
            data->dpi = dpi.first ? dpi : deviceDPI(hdc);
#else
            data->dpi = deviceDPI(hdc);
#endif
            data->depth = GetDeviceCaps(hdc, BITSPIXEL);
            data->format = data->depth == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32;
            data->physicalSizeMM = QSizeF(GetDeviceCaps(hdc, HORZSIZE), GetDeviceCaps(hdc, VERTSIZE));
            const int refreshRate = GetDeviceCaps(hdc, VREFRESH);
            if (refreshRate > 1) // 0,1 means hardware default.
                data->refreshRateHz = refreshRate;
            DeleteDC(hdc);
        } else {
            qWarning("%s: Unable to obtain handle for monitor '%s', defaulting to %g DPI.",
                     __FUNCTION__, qPrintable(QString::fromWCharArray(info.szDevice)),
                     data->dpi.first);
        } // CreateDC() failed
    } // not lock screen
    data->orientation = data->geometry.height() > data->geometry.width() ?
                       Qt::PortraitOrientation : Qt::LandscapeOrientation;
    // EnumDisplayMonitors (as opposed to EnumDisplayDevices) enumerates only
    // virtual desktop screens.
    data->flags |= QWindowsScreenData::VirtualDesktop;
    if (info.dwFlags & MONITORINFOF_PRIMARY)
        data->flags |= QWindowsScreenData::PrimaryScreen;
    return true;
}

// from QDesktopWidget, taking WindowsScreenDataList as LPARAM
BOOL QT_WIN_CALLBACK monitorEnumCallback(HMONITOR hMonitor, HDC, LPRECT, LPARAM p)
{
    QWindowsScreenData data;
    if (monitorData(hMonitor, &data)) {
        WindowsScreenDataList *result = reinterpret_cast<WindowsScreenDataList *>(p);
        // QPlatformIntegration::screenAdded() documentation specifies that first
        // added screen will be the primary screen, so order accordingly.
        // Note that the side effect of this policy is that there is no way to change primary
        // screen reported by Qt, unless we want to delete all existing screens and add them
        // again whenever primary screen changes.
        if (data.flags & QWindowsScreenData::PrimaryScreen)
            result->prepend(data);
        else
            result->append(data);
    }
    return TRUE;
}

static inline WindowsScreenDataList monitorData()
{
    WindowsScreenDataList result;
    EnumDisplayMonitors(0, 0, monitorEnumCallback, (LPARAM)&result);
    return result;
}

static QDebug operator<<(QDebug dbg, const QWindowsScreenData &d)
{
    QDebug nospace =  dbg.nospace();
    nospace << "Screen " << d.name << ' '
            << d.geometry.width() << 'x' << d.geometry.height() << '+' << d.geometry.x() << '+' << d.geometry.y()
            << " avail: "
            << d.availableGeometry.width() << 'x' << d.availableGeometry.height() << '+' << d.availableGeometry.x() << '+' << d.availableGeometry.y()
            << " physical: " << d.physicalSizeMM.width() <<  'x' << d.physicalSizeMM.height()
            << " DPI: " << d.dpi.first << 'x' << d.dpi.second << " Depth: " << d.depth
            << " Format: " << d.format;
    if (d.flags & QWindowsScreenData::PrimaryScreen)
        nospace << " primary";
    if (d.flags & QWindowsScreenData::VirtualDesktop)
        nospace << " virtual desktop";
    if (d.flags & QWindowsScreenData::LockScreen)
        nospace << " lock screen";
    return dbg;
}

// Return the cursor to be shared by all screens (virtual desktop).
static inline QSharedPointer<QWindowsCursor> sharedCursor()
{
#ifndef QT_NO_CURSOR
    if (const QScreen *primaryScreen = QGuiApplication::primaryScreen())
        return static_cast<const QWindowsScreen *>(primaryScreen->handle())->windowsCursor();
#endif
    return QSharedPointer<QWindowsCursor>(new QWindowsCursor);
}

/*!
    \class QWindowsScreen
    \brief Windows screen.
    \sa QWindowsScreenManager
    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsScreen::QWindowsScreen(const QWindowsScreenData &data) :
    m_data(data)
#ifndef QT_NO_CURSOR
    ,m_cursor(sharedCursor())
#endif
{
}

BOOL QT_WIN_CALLBACK monitorResolutionEnumCallback(HMONITOR hMonitor, HDC, LPRECT, LPARAM p)
{
    QWindowsScreenData data;
    if (monitorData(hMonitor, &data)) {
        int *maxHorizResolution = reinterpret_cast<int *>(p);
        const int horizResolution = qRound(data.dpi.first);
        if (horizResolution > *maxHorizResolution)
            *maxHorizResolution = horizResolution;
    }
    return TRUE;
}

int QWindowsScreen::maxMonitorHorizResolution()
{
    int result = 0;
    EnumDisplayMonitors(0, 0, monitorResolutionEnumCallback, (LPARAM)&result);
    return result;
}

Q_GUI_EXPORT QPixmap qt_pixmapFromWinHBITMAP(HBITMAP bitmap, int hbitmapFormat = 0);

QPixmap QWindowsScreen::grabWindow(WId window, int qX, int qY, int qWidth, int qHeight) const
{
    RECT r;
    HWND hwnd = window ? (HWND)window : GetDesktopWindow();
    GetClientRect(hwnd, &r);
    const int x = qX * QWindowsScaling::factor();
    const int y = qY * QWindowsScaling::factor();
    int width = qWidth * QWindowsScaling::factor();
    int height = qHeight * QWindowsScaling::factor();
    if (width < 0) width = r.right - r.left;
    if (height < 0) height = r.bottom - r.top;

    // Create and setup bitmap
    HDC display_dc = GetDC(0);
    HDC bitmap_dc = CreateCompatibleDC(display_dc);
    HBITMAP bitmap = CreateCompatibleBitmap(display_dc, width, height);
    HGDIOBJ null_bitmap = SelectObject(bitmap_dc, bitmap);

    // copy data
    HDC window_dc = GetDC(hwnd);
    BitBlt(bitmap_dc, 0, 0, width, height, window_dc, x, y, SRCCOPY | CAPTUREBLT);

    // clean up all but bitmap
    ReleaseDC(hwnd, window_dc);
    SelectObject(bitmap_dc, null_bitmap);
    DeleteDC(bitmap_dc);

    const QPixmap pixmap = qt_pixmapFromWinHBITMAP(bitmap);

    DeleteObject(bitmap);
    ReleaseDC(0, display_dc);

    if (QWindowsScaling::isActive()) {
        const qreal factor = 1.0 / qreal(QWindowsScaling::factor());
        return pixmap.transformed(QTransform::fromScale(factor, factor));
    }
    return pixmap;
}

/*!
    \brief Find a top level window taking the flags of ChildWindowFromPointEx.
*/

QWindow *QWindowsScreen::topLevelAt(const QPoint &point) const
{
    QWindow *result = 0;
    if (QWindow *child = QWindowsScreen::windowAt(point * QWindowsScaling::factor(), CWP_SKIPINVISIBLE))
        result = QWindowsWindow::topLevelOf(child);
    qCDebug(lcQpaWindows) <<__FUNCTION__ << point << result;
    return result;
}

QWindow *QWindowsScreen::windowAt(const QPoint &screenPoint, unsigned flags)
{
    QWindow* result = 0;
    if (QPlatformWindow *bw = QWindowsContext::instance()->
            findPlatformWindowAt(GetDesktopWindow(), screenPoint, flags))
        result = bw->window();
    qCDebug(lcQpaWindows) <<__FUNCTION__ << screenPoint << " returns " << result;
    return result;
}

QWindowsScreen *QWindowsScreen::screenOf(const QWindow *w)
{
    if (w)
        if (const QScreen *s = w->screen())
            if (QPlatformScreen *pscr = s->handle())
                return static_cast<QWindowsScreen *>(pscr);
    if (const QScreen *ps = QGuiApplication::primaryScreen())
        if (QPlatformScreen *ppscr = ps->handle())
            return static_cast<QWindowsScreen *>(ppscr);
    return 0;
}

/*!
    \brief Determine siblings in a virtual desktop system.

    Self is by definition a sibling, else collect all screens
    within virtual desktop.
*/

QList<QPlatformScreen *> QWindowsScreen::virtualSiblings() const
{
    QList<QPlatformScreen *> result;
    if (m_data.flags & QWindowsScreenData::VirtualDesktop) {
        foreach (QWindowsScreen *screen, QWindowsContext::instance()->screenManager().screens())
            if (screen->data().flags & QWindowsScreenData::VirtualDesktop)
                result.push_back(screen);
    } else {
        result.push_back(const_cast<QWindowsScreen *>(this));
    }
    return result;
}

/*!
    \brief Notify QWindowSystemInterface about changes of a screen and synchronize data.
*/

void QWindowsScreen::handleChanges(const QWindowsScreenData &newData)
{
    m_data.physicalSizeMM = newData.physicalSizeMM;

    if (m_data.geometry != newData.geometry || m_data.availableGeometry != newData.availableGeometry) {
        m_data.geometry = newData.geometry;
        m_data.availableGeometry = newData.availableGeometry;
        QWindowSystemInterface::handleScreenGeometryChange(screen(),
                                                           newData.geometry, newData.availableGeometry);
    }
    if (!qFuzzyCompare(m_data.dpi.first, newData.dpi.first)
        || !qFuzzyCompare(m_data.dpi.second, newData.dpi.second)) {
        m_data.dpi = newData.dpi;
        QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(screen(),
                                                                     newData.dpi.first,
                                                                     newData.dpi.second);
    }
    if (m_data.orientation != newData.orientation) {
        m_data.orientation = newData.orientation;
        QWindowSystemInterface::handleScreenOrientationChange(screen(),
                                                              newData.orientation);
    }
}

/*!
    \class QWindowsScreenManager
    \brief Manages a list of QWindowsScreen.

    Listens for changes and notifies QWindowSystemInterface about changed/
    added/deleted screens.

    \sa QWindowsScreen
    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsScreenManager::QWindowsScreenManager() :
    m_lastDepth(-1), m_lastHorizontalResolution(0), m_lastVerticalResolution(0)
{
}

/*!
    \brief Triggers synchronization of screens (WM_DISPLAYCHANGE).

    Subsequent events are compressed since WM_DISPLAYCHANGE is sent to
    each top level window.
*/

bool QWindowsScreenManager::handleDisplayChange(WPARAM wParam, LPARAM lParam)
{
    const int newDepth = (int)wParam;
    const WORD newHorizontalResolution = LOWORD(lParam);
    const WORD newVerticalResolution = HIWORD(lParam);
    if (newDepth != m_lastDepth || newHorizontalResolution != m_lastHorizontalResolution
        || newVerticalResolution != m_lastVerticalResolution) {
        m_lastDepth = newDepth;
        m_lastHorizontalResolution = newHorizontalResolution;
        m_lastVerticalResolution = newVerticalResolution;
        qCDebug(lcQpaWindows) << __FUNCTION__ << "Depth=" << newDepth
            << ", resolution " << newHorizontalResolution << 'x' << newVerticalResolution;
        handleScreenChanges();
    }
    return false;
}

static inline int indexOfMonitor(const QList<QWindowsScreen *> &screens,
                                 const QString &monitorName)
{
    for (int i= 0; i < screens.size(); ++i)
        if (screens.at(i)->data().name == monitorName)
            return i;
    return -1;
}

static inline int indexOfMonitor(const QList<QWindowsScreenData> &screenData,
                                 const QString &monitorName)
{
    for (int i = 0; i < screenData.size(); ++i)
        if (screenData.at(i).name == monitorName)
            return i;
    return -1;
}

// Move a window to a new virtual screen, accounting for varying sizes.
static void moveToVirtualScreen(QWindow *w, const QScreen *newScreen)
{
    QRect geometry = w->geometry();
    const QRect oldScreenGeometry = w->screen()->geometry();
    const QRect newScreenGeometry = newScreen->geometry();
    QPoint relativePosition = geometry.topLeft() - oldScreenGeometry.topLeft();
    if (oldScreenGeometry.size() != newScreenGeometry.size()) {
        const qreal factor =
            qreal(QPoint(newScreenGeometry.width(), newScreenGeometry.height()).manhattanLength()) /
            qreal(QPoint(oldScreenGeometry.width(), oldScreenGeometry.height()).manhattanLength());
        relativePosition = (QPointF(relativePosition) * factor).toPoint();
    }
    geometry.moveTopLeft(relativePosition);
    w->setGeometry(geometry);
}

void QWindowsScreenManager::removeScreen(int index)
{
    qCDebug(lcQpaWindows) << "Removing Monitor:" << m_screens.at(index)->data();
    QScreen *screen = m_screens.at(index)->screen();
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    // QTBUG-38650: When a screen is disconnected, Windows will automatically
    // move the Window to another screen. This will trigger a geometry change
    // event, but unfortunately after the screen destruction signal. To prevent
    // QtGui from automatically hiding the QWindow, pretend all Windows move to
    // the primary screen first (which is likely the correct, final screen).
    // QTBUG-39320: Windows does not automatically move WS_EX_TOOLWINDOW (dock) windows;
    // move those manually.
    if (screen != primaryScreen) {
        unsigned movedWindowCount = 0;
        foreach (QWindow *w, QGuiApplication::topLevelWindows()) {
            if (w->screen() == screen && w->handle() && w->type() != Qt::Desktop) {
                if (w->isVisible() && w->windowState() != Qt::WindowMinimized
                    && (QWindowsWindow::baseWindowOf(w)->exStyle() & WS_EX_TOOLWINDOW)) {
                    moveToVirtualScreen(w, primaryScreen);
                } else {
                    QWindowSystemInterface::handleWindowScreenChanged(w, primaryScreen);
                }
                ++movedWindowCount;
            }
        }
        if (movedWindowCount)
            QWindowSystemInterface::flushWindowSystemEvents();
    }
    delete m_screens.takeAt(index);
}

/*!
    \brief Synchronizes the screen list, adds new screens, removes deleted
    ones and propagates resolution changes to QWindowSystemInterface.
*/

bool QWindowsScreenManager::handleScreenChanges()
{
    // Look for changed monitors, add new ones
    WindowsScreenDataList newDataList = monitorData();
    const bool lockScreen = newDataList.size() == 1 && (newDataList.front().flags & QWindowsScreenData::LockScreen);
    foreach (const QWindowsScreenData &newData, newDataList) {
        const int existingIndex = indexOfMonitor(m_screens, newData.name);
        if (existingIndex != -1) {
            m_screens.at(existingIndex)->handleChanges(newData);
        } else {
            QWindowsScreen *newScreen = new QWindowsScreen(newData);
            m_screens.push_back(newScreen);
            QWindowsIntegration::instance()->emitScreenAdded(newScreen);
            qCDebug(lcQpaWindows) << "New Monitor: " << newData;
        }    // exists
    }        // for new screens.
    // Remove deleted ones but keep main monitors if we get only the
    // temporary lock screen to avoid window recreation (QTBUG-33062).
    if (!lockScreen) {
        for (int i = m_screens.size() - 1; i >= 0; --i) {
            if (indexOfMonitor(newDataList, m_screens.at(i)->data().name) == -1)
                removeScreen(i);
        }     // for existing screens
    }     // not lock screen
    return true;
}

QT_END_NAMESPACE
