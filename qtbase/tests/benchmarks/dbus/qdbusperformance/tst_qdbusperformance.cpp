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
#include <QtCore/QtCore>
#include <QtTest/QtTest>
#include <QtDBus/QtDBus>

#include "./serverobject.h"

static const char serviceName[] = "org.qtproject.autotests.performance";
static const int runTime = 500;

class tst_QDBusPerformance: public QObject
{
    Q_OBJECT
    QProcess proc;
    QDBusInterface *target;

    QDBusInterface *remote;
    QDBusInterface *local;

    bool executeTest(const char *funcname, int size, const QVariant &data);

public slots:
    void initTestCase_data();
    void initTestCase();
    void init();

private slots:
    void callSpeed();

    void oneWay_data();
    void oneWay();
    void oneWayVariant_data();
    void oneWayVariant();

    void roundTrip_data();
    void roundTrip();
    void roundTripVariant_data();
    void roundTripVariant();
};

void tst_QDBusPerformance::initTestCase()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QDBusServiceWatcher watcher(serviceName, con,
                                QDBusServiceWatcher::WatchForRegistration);
    connect(&watcher, SIGNAL(serviceRegistered(QString)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));

#ifdef Q_OS_WIN
#  define EXE ".exe"
#else
#  define EXE ""
#endif
    proc.start(QFINDTESTDATA("server/server" EXE));
    QVERIFY2(proc.waitForStarted(), qPrintable(proc.errorString()));
    QVERIFY(proc.waitForReadyRead());

    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(con.interface()->isServiceRegistered(serviceName));

    remote = new QDBusInterface(serviceName, "/", "org.qtproject.autotests.Performance", con, this);
    QVERIFY(remote->isValid());

    new ServerObject("/", con, this);
    local = new QDBusInterface(con.baseService(), "/", "org.qtproject.autotests.Performance", con, this);
    QVERIFY(local->isValid());
}

void tst_QDBusPerformance::initTestCase_data()
{
    QTest::addColumn<bool>("loopback");

    QTest::newRow("normal") << false;
    QTest::newRow("loopback") << true;
}

void tst_QDBusPerformance::init()
{
    QFETCH_GLOBAL(bool, loopback);
    if (loopback)
        target = local;
    else
        target = remote;
}

void tst_QDBusPerformance::callSpeed()
{
    QElapsedTimer timer;

    int callCount = 0;
    timer.start();
    while (timer.elapsed() < runTime) {
        QDBusReply<void> reply = target->call("nothing");
        QVERIFY(reply.isValid());

        ++callCount;
    }
    qDebug() << callCount << "calls in" << timer.elapsed() << "ms:"
             << (callCount * 1000.0 / timer.elapsed()) << "calls/sec";
}

bool tst_QDBusPerformance::executeTest(const char *funcname, int size, const QVariant &data)
{
    QElapsedTimer timer;

    int callCount = 0;
    qint64 transferred = 0;
    timer.start();
    while (timer.elapsed() < runTime) {
        QDBusMessage reply = target->call(funcname, data);
        if (reply.type() != QDBusMessage::ReplyMessage)
            return false;

        transferred += size;
        ++callCount;
    }
    qDebug() << transferred << "bytes in" << timer.elapsed() << "ms"
             << "(in" << callCount << "calls):"
             << (transferred * 1000.0 / timer.elapsed() / 1024 / 1024) << "MB/s";

    return true;
}

void tst_QDBusPerformance::oneWay_data()
{
    QTest::addColumn<QVariant>("data");
    QTest::addColumn<int>("size");

    QByteArray ba(256, 'a');
    while (ba.size() < 8193) {
        QTest::newRow(QString("%1-byteArray").arg(ba.size()).toLatin1()) << QVariant::fromValue(ba) << ba.size();
        ba += ba;
    }

    QString s(256, QLatin1Char('a'));
    while (s.size() < 8193) {
        QTest::newRow(QString("%1-string").arg(s.size()).toLatin1()) << QVariant::fromValue(s) << s.size();
        s += s;
    }
}

void tst_QDBusPerformance::oneWay()
{
    QFETCH(QVariant, data);
    QFETCH(int, size);

    QVERIFY(executeTest("size", size, data));
}

void tst_QDBusPerformance::oneWayVariant_data()
{
    oneWay_data();
}

void tst_QDBusPerformance::oneWayVariant()
{
    QFETCH(QVariant, data);
    QFETCH(int, size);

    QVERIFY(executeTest("size", size, QVariant::fromValue(QDBusVariant(data))));
}

void tst_QDBusPerformance::roundTrip_data()
{
    oneWay_data();
}

void tst_QDBusPerformance::roundTrip()
{
    QFETCH(QVariant, data);
    QFETCH(int, size);

    QVERIFY(executeTest("echo", size, data));
}

void tst_QDBusPerformance::roundTripVariant_data()
{
    oneWay_data();
}

void tst_QDBusPerformance::roundTripVariant()
{
    QFETCH(QVariant, data);
    QFETCH(int, size);

    QVERIFY(executeTest("echo", size, QVariant::fromValue(QDBusVariant(data))));
}

QTEST_MAIN(tst_QDBusPerformance)
#include "tst_qdbusperformance.moc"
