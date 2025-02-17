/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtBluetooth module of the Qt Toolkit.
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

#include <QtCore/QLoggingCategory>
#include <QtAndroidExtras/QAndroidJniEnvironment>

#include "android/serveracceptancethread_p.h"

Q_DECLARE_LOGGING_CATEGORY(QT_BT_ANDROID)

ServerAcceptanceThread::ServerAcceptanceThread(QObject *parent) :
    QObject(parent), maxPendingConnections(1)
{
    qRegisterMetaType<QBluetoothServer::Error>("QBluetoothServer::Error");
}

ServerAcceptanceThread::~ServerAcceptanceThread()
{
    Q_ASSERT(!isRunning());
    QMutexLocker lock(&m_mutex);
    shutdownPendingConnections();
}

void ServerAcceptanceThread::setServiceDetails(const QBluetoothUuid &uuid,
                                               const QString &serviceName,
                                               QBluetooth::SecurityFlags securityFlags)
{
    QMutexLocker lock(&m_mutex);
    m_uuid = uuid;
    m_serviceName = serviceName;
    secFlags = securityFlags;
}

bool ServerAcceptanceThread::hasPendingConnections() const
{
    QMutexLocker lock(&m_mutex);
    return (pendingSockets.count() > 0);
}

/*
 * Returns the next pending connection or an invalid JNI object.
 * Note that even a stopped thread may still have pending
 * connections. Pending connections are only terminated upon
 * thread restart or destruction.
 */
QAndroidJniObject ServerAcceptanceThread::nextPendingConnection()
{
    QMutexLocker lock(&m_mutex);
    if (pendingSockets.isEmpty())
        return QAndroidJniObject();
    else
        return pendingSockets.takeFirst();
}

void ServerAcceptanceThread::setMaxPendingConnections(int maximumCount)
{
    QMutexLocker lock(&m_mutex);
    maxPendingConnections = maximumCount;
}

void ServerAcceptanceThread::run()
{
    QMutexLocker lock(&m_mutex);

    if (!validSetup()) {
        qCWarning(QT_BT_ANDROID) << "Invalid Server Socket setup";
        return;
    }

    if (isRunning()) {
        stop();
        shutdownPendingConnections();
    }

    javaThread = QAndroidJniObject("org/qtproject/qt5/android/bluetooth/QtBluetoothSocketServer");
    if (!javaThread.isValid())
        return;

    javaThread.setField<jlong>("qtObject", reinterpret_cast<long>(this));
    javaThread.setField<jboolean>("logEnabled", QT_BT_ANDROID().isDebugEnabled());

    QString tempUuid = m_uuid.toString();
    tempUuid.chop(1); //remove trailing '}'
    tempUuid.remove(0,1); //remove first '{'

    QAndroidJniObject uuidString = QAndroidJniObject::fromString(tempUuid);
    QAndroidJniObject serviceNameString = QAndroidJniObject::fromString(m_serviceName);
    bool isSecure = !(secFlags == QBluetooth::NoSecurity);
    javaThread.callMethod<void>("setServiceDetails", "(Ljava/lang/String;Ljava/lang/String;Z)V",
                                uuidString.object<jstring>(),
                                serviceNameString.object<jstring>(),
                                isSecure);
    javaThread.callMethod<void>("start");
}

void ServerAcceptanceThread::stop()
{
    if (javaThread.isValid()) {
        qCDebug(QT_BT_ANDROID) << "Closing server socket";
        javaThread.callMethod<void>("close");
    }
}

bool ServerAcceptanceThread::isRunning() const
{
    if (javaThread.isValid())
        return javaThread.callMethod<jboolean>("isAlive");

    return false;
}

//Runs inside the java thread
void ServerAcceptanceThread::javaThreadErrorOccurred(int errorCode)
{
    qCDebug(QT_BT_ANDROID) << "JavaThread error:" << errorCode;
    emit error(QBluetoothServer::InputOutputError);
}

//Runs inside the Java thread
void ServerAcceptanceThread::javaNewSocket(jobject s)
{
    QMutexLocker lock(&m_mutex);

    QAndroidJniObject socket(s);
    if (!socket.isValid())
       return;

    if (pendingSockets.count() < maxPendingConnections) {
        qCDebug(QT_BT_ANDROID) << "New incoming java socket detected";
        pendingSockets.append(socket);
        emit newConnection();
    } else {
        QAndroidJniEnvironment env;
        qCWarning(QT_BT_ANDROID) << "Refusing connection due to limited pending socket queue";
        socket.callMethod<void>("close");
        if (env->ExceptionCheck()) {
            qCWarning(QT_BT_ANDROID) << "Error during refusal of new socket";
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
    }
}

bool ServerAcceptanceThread::validSetup() const
{
    return (!m_uuid.isNull() && !m_serviceName.isEmpty());
}

void ServerAcceptanceThread::shutdownPendingConnections()
{
    while (!pendingSockets.isEmpty()) {
        QAndroidJniObject socket = pendingSockets.takeFirst();
        socket.callMethod<void>("close");
    }
}
