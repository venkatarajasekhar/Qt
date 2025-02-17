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
// Qt
#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QTimer>

// Test
#include "Test.h"

//------------------------------------------------------------------------------
My4Socket::My4Socket(QObject *parent)
    : QTcpSocket(parent), safeShutDown(false)
{
    connect(this, SIGNAL(readyRead()), this, SLOT(read()));
    connect(this, SIGNAL(disconnected()), this, SLOT(closed()));
}

//------------------------------------------------------------------------------
void My4Socket::read(void)
{
    QDataStream in(this);

    quint32 num, reply;

    while (bytesAvailable()) {
        in >> num;
        if (num == 42) {
            safeShutDown = true;
            qDebug("SUCCESS");
            QCoreApplication::instance()->quit();
            return;
        }
        reply = num + 1;
        if (reply == 42)
            ++reply;
    }

    // Reply with a bigger number
    sendTest(reply);
}

//------------------------------------------------------------------------------
void My4Socket::closed(void)
{
    if (!safeShutDown)
        qDebug("FAILED");
    QCoreApplication::instance()->quit();
}

//------------------------------------------------------------------------------
void My4Socket::sendTest(quint32 num)
{
    QByteArray  block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << num;

    write(block, block.size());
}

//------------------------------------------------------------------------------
My4Server::My4Server(QObject *parent)
    : QTcpServer(parent)
{
    if (listen(QHostAddress::Any, 7700))
        qDebug("qt4server");
    QTimer::singleShot(5000, this, SLOT(stopServer()));
}

//------------------------------------------------------------------------------
void My4Server::incomingConnection(qintptr socketId)
{
    m_socket = new My4Socket(this);
    m_socket->setSocketDescriptor(socketId);
}

//------------------------------------------------------------------------------
void My4Server::stopServer()
{
    if (m_socket) {
        qDebug("SUCCESS");
        m_socket->safeShutDown = true;
        m_socket->sendTest(42);
    } else {
        QCoreApplication::instance()->quit();
    }
}

//------------------------------------------------------------------------------
Test::Test(Type type)
{
    switch (type) {
    case Qt4Client: {
        qDebug("qt4client");
        My4Socket *s = new My4Socket(this);
        s->connectToHost("localhost", 7700);
        s->sendTest(1);
        break;
    }
    case Qt4Server: {
        new My4Server(this);
        break;
    }
    default:
        break;
    }
}
