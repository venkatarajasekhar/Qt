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

#include "qwindowsbackingstore.h"
#include "qwindowswindow.h"
#include "qwindowsnativeimage.h"
#include "qwindowscontext.h"
#include "qwindowsscaling.h"

#include <QtGui/QWindow>
#include <QtGui/QPainter>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsBackingStore
    \brief Backing store for windows.
    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsBackingStore::QWindowsBackingStore(QWindow *window) :
    QPlatformBackingStore(window)
{
    qCDebug(lcQpaBackingStore) << __FUNCTION__ << this << window;
}

QWindowsBackingStore::~QWindowsBackingStore()
{
    qCDebug(lcQpaBackingStore) << __FUNCTION__ << this;
}

QPaintDevice *QWindowsBackingStore::paintDevice()
{
    Q_ASSERT(!m_image.isNull());
    return &m_image->image();
}

void QWindowsBackingStore::flushDp(QWindow *window, const QRect &br, const QPoint &offset)
{
    Q_ASSERT(window);

    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaBackingStore) << __FUNCTION__ << this << window << offset << br;
    QWindowsWindow *rw = QWindowsWindow::baseWindowOf(window);

#ifndef Q_OS_WINCE
    const bool hasAlpha = rw->format().hasAlpha();
    const Qt::WindowFlags flags = window->flags();
    if ((flags & Qt::FramelessWindowHint) && QWindowsWindow::setWindowLayered(rw->handle(), flags, hasAlpha, rw->opacity()) && hasAlpha) {
        // Windows with alpha: Use blend function to update.
        const QMargins marginsDP = rw->frameMarginsDp();
        const QRect r = rw->geometryDp() + marginsDP;
        const QPoint frameOffset(marginsDP.left(), marginsDP.top());
        QRect dirtyRect = br.translated(offset + frameOffset);

        SIZE size = {r.width(), r.height()};
        POINT ptDst = {r.x(), r.y()};
        POINT ptSrc = {0, 0};
        BLENDFUNCTION blend = {AC_SRC_OVER, 0, (BYTE)(255.0 * rw->opacity()), AC_SRC_ALPHA};
        if (QWindowsContext::user32dll.updateLayeredWindowIndirect) {
            RECT dirty = {dirtyRect.x(), dirtyRect.y(),
                dirtyRect.x() + dirtyRect.width(), dirtyRect.y() + dirtyRect.height()};
            UPDATELAYEREDWINDOWINFO info = {sizeof(info), NULL, &ptDst, &size, m_image->hdc(), &ptSrc, 0, &blend, ULW_ALPHA, &dirty};
            QWindowsContext::user32dll.updateLayeredWindowIndirect(rw->handle(), &info);
        } else {
            QWindowsContext::user32dll.updateLayeredWindow(rw->handle(), NULL, &ptDst, &size, m_image->hdc(), &ptSrc, 0, &blend, ULW_ALPHA);
        }
    } else {
#endif
        const HDC dc = rw->getDC();
        if (!dc) {
            qErrnoWarning("%s: GetDC failed", __FUNCTION__);
            return;
        }

        if (!BitBlt(dc, br.x(), br.y(), br.width(), br.height(),
                    m_image->hdc(), br.x() + offset.x(), br.y() + offset.y(), SRCCOPY)) {
            const DWORD lastError = GetLastError(); // QTBUG-35926, QTBUG-29716: may fail after lock screen.
            if (lastError != ERROR_SUCCESS && lastError != ERROR_INVALID_HANDLE)
                qErrnoWarning(lastError, "%s: BitBlt failed", __FUNCTION__);
        }
        rw->releaseDC();
#ifndef Q_OS_WINCE
    }
#endif

    // Write image for debug purposes.
    if (QWindowsContext::verbose > 2 && lcQpaBackingStore().isDebugEnabled()) {
        static int n = 0;
        const QString fileName = QString::fromLatin1("win%1_%2.png").
                arg(rw->winId()).arg(n++);
        m_image->image().save(fileName);
        qCDebug(lcQpaBackingStore) << "Wrote " << m_image->image().size() << fileName;
    }
}

void QWindowsBackingStore::resize(const QSize &sizeDip, const QRegion &regionDip)
{
    const QSize size = sizeDip * QWindowsScaling::factor();
    if (m_image.isNull() || m_image->image().size() != size) {
#ifndef QT_NO_DEBUG_OUTPUT
        if (QWindowsContext::verbose && lcQpaBackingStore().isDebugEnabled()) {
            qCDebug(lcQpaBackingStore)
                << __FUNCTION__ << ' ' << window() << ' ' << size << ' ' << sizeDip << ' '
                << regionDip << " from: " << (m_image.isNull() ? QSize() : m_image->image().size());
        }
#endif
        const QImage::Format format = window()->format().hasAlpha() ?
            QImage::Format_ARGB32_Premultiplied : QWindowsNativeImage::systemFormat();

        QWindowsNativeImage *oldwni = m_image.data();
        QWindowsNativeImage *newwni = new QWindowsNativeImage(size.width(), size.height(), format);

        if (oldwni && !regionDip.isEmpty()) {
            const QImage &oldimg(oldwni->image());
            QImage &newimg(newwni->image());
            QRegion staticRegion = QWindowsScaling::mapToNative(regionDip);
            staticRegion &= QRect(0, 0, oldimg.width(), oldimg.height());
            staticRegion &= QRect(0, 0, newimg.width(), newimg.height());
            QPainter painter(&newimg);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            foreach (const QRect &rect, staticRegion.rects())
                painter.drawImage(rect, oldimg, rect);
        }

        if (QWindowsScaling::isActive())
            newwni->setDevicePixelRatio(QWindowsScaling::factor());
        m_image.reset(newwni);
    }
}

Q_GUI_EXPORT void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

bool QWindowsBackingStore::scroll(const QRegion &areaDip, int dxDip, int dyDip)
{
    if (m_image.isNull() || m_image->image().isNull())
        return false;

    const QPoint dp = QPoint(dxDip, dyDip) * QWindowsScaling::factor();
    const QVector<QRect> rects = areaDip.rects();
    for (int i = 0; i < rects.size(); ++i)
        qt_scrollRectInImage(m_image->image(), QWindowsScaling::mapToNative(rects.at(i)), dp);

    return true;
}

void QWindowsBackingStore::beginPaint(const QRegion &regionDip)
{
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaBackingStore) <<__FUNCTION__ << regionDip;

    if (m_image->image().hasAlphaChannel()) {
        QPainter p(&m_image->image());
        p.setCompositionMode(QPainter::CompositionMode_Source);
        const QColor blank = Qt::transparent;
        foreach (const QRect &r, regionDip.rects())
            p.fillRect(QWindowsScaling::mapToNative(r), blank);
    }
}

HDC QWindowsBackingStore::getDC() const
{
    if (!m_image.isNull())
        return m_image->hdc();
    return 0;
}

#ifndef QT_NO_OPENGL

QImage QWindowsBackingStore::toImage() const
{
    if (m_image.isNull()) {
        qCWarning(lcQpaBackingStore) <<__FUNCTION__ << "Image is null.";
        return QImage();
    }
    return m_image.data()->image();
}

#endif // !QT_NO_OPENGL

QT_END_NAMESPACE
