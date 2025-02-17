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

#ifndef QCOCOAGLCONTEXT_H
#define QCOCOAGLCONTEXT_H

#include <QtCore/QPointer>
#include <qpa/qplatformopenglcontext.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QWindow>

#undef slots
#include <Cocoa/Cocoa.h>

QT_BEGIN_NAMESPACE

class QCocoaGLContext : public QPlatformOpenGLContext
{
public:
    QCocoaGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, const QVariant &nativeHandle);
    ~QCocoaGLContext();

    QSurfaceFormat format() const;

    void swapBuffers(QPlatformSurface *surface);

    bool makeCurrent(QPlatformSurface *surface);
    void doneCurrent();

    void (*getProcAddress(const QByteArray &procName)) ();

    void update();

    static NSOpenGLPixelFormat *createNSOpenGLPixelFormat(const QSurfaceFormat &format);
    NSOpenGLContext *nsOpenGLContext() const;

    bool isSharing() const;
    bool isValid() const;

    void windowWasHidden();

    QVariant nativeHandle() const;

private:
    void setActiveWindow(QWindow *window);
    void updateSurfaceFormat();

    NSOpenGLContext *m_context;
    NSOpenGLContext *m_shareContext;
    QSurfaceFormat m_format;
    QPointer<QWindow> m_currentWindow;
};

QT_END_NAMESPACE

#endif // QCOCOAGLCONTEXT_H
