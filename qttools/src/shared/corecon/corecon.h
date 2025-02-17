/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CORECON_H
#define CORECON_H

#include <QtCore/qt_windows.h>
#include <QtCore/QList>
#include <QtCore/QScopedPointer>
#include <QtCore/QLoggingCategory>

QT_USE_NAMESPACE

class CoreConDevicePrivate;
class CoreConDevice
{
public:
    explicit CoreConDevice(int version);
    ~CoreConDevice();
    QString name() const;
    QString id() const;
    bool isEmulator() const;
    Qt::HANDLE handle() const;
private:
    QScopedPointer<CoreConDevicePrivate> d_ptr;
    Q_DECLARE_PRIVATE(CoreConDevice)
friend class CoreConServerPrivate;
};

class CoreConServerPrivate;
class CoreConServer
{
public:
    explicit CoreConServer(int version);
    ~CoreConServer();
    bool initialize();
    Qt::HANDLE handle() const;
    QList<CoreConDevice *> devices() const;
    QString formatError(HRESULT hr) const;
private:
    QScopedPointer<CoreConServerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(CoreConServer)
};

Q_DECLARE_LOGGING_CATEGORY(lcCoreCon)

#endif // CORECON_H
