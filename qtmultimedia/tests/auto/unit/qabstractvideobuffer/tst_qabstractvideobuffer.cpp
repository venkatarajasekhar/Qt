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

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>

#include <qabstractvideobuffer.h>

// Adds an enum, and the stringized version
#define ADD_ENUM_TEST(x) \
    QTest::newRow(#x) \
        << QAbstractVideoBuffer::x \
    << QString(QLatin1String(#x));

class tst_QAbstractVideoBuffer : public QObject
{
    Q_OBJECT
public:
    tst_QAbstractVideoBuffer();
    ~tst_QAbstractVideoBuffer();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void handleType_data();
    void handleType();
    void handle();
    void mapMode();
    void mapModeDebug_data();
    void mapModeDebug();
};

class QtTestVideoBuffer : public QAbstractVideoBuffer
{
public:
    QtTestVideoBuffer(QAbstractVideoBuffer::HandleType type) : QAbstractVideoBuffer(type) {}

    MapMode mapMode() const { return QAbstractVideoBuffer::ReadWrite; }

    uchar *map(MapMode, int *, int *) { return 0; }
    void unmap() {}
};

tst_QAbstractVideoBuffer::tst_QAbstractVideoBuffer()
{
}

tst_QAbstractVideoBuffer::~tst_QAbstractVideoBuffer()
{
}

void tst_QAbstractVideoBuffer::initTestCase()
{
}

void tst_QAbstractVideoBuffer::cleanupTestCase()
{
}

void tst_QAbstractVideoBuffer::init()
{
}

void tst_QAbstractVideoBuffer::cleanup()
{
}

void tst_QAbstractVideoBuffer::handleType_data()
{
    QTest::addColumn<QAbstractVideoBuffer::HandleType>("type");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(NoHandle);
    ADD_ENUM_TEST(GLTextureHandle);
    ADD_ENUM_TEST(XvShmImageHandle);
    ADD_ENUM_TEST(QPixmapHandle);
    ADD_ENUM_TEST(CoreImageHandle);

    // User handles are different

    QTest::newRow("user1")
            << QAbstractVideoBuffer::UserHandle << QString::fromLatin1("UserHandle(1000)");
    QTest::newRow("user2")
            << QAbstractVideoBuffer::HandleType(QAbstractVideoBuffer::UserHandle + 1) << QString::fromLatin1("UserHandle(1001)");
}

void tst_QAbstractVideoBuffer::handleType()
{
    QFETCH(QAbstractVideoBuffer::HandleType, type);
    QFETCH(QString, stringized);

    QtTestVideoBuffer buffer(type);

    QCOMPARE(buffer.handleType(), type);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << type;
}

void tst_QAbstractVideoBuffer::handle()
{
    QtTestVideoBuffer buffer(QAbstractVideoBuffer::NoHandle);

    QVERIFY(buffer.handle().isNull());
}

void tst_QAbstractVideoBuffer::mapMode()
{
    QtTestVideoBuffer maptest(QAbstractVideoBuffer::NoHandle);
    QVERIFY2(maptest.mapMode() == QAbstractVideoBuffer::ReadWrite, "ReadWrite Failed");
}

void tst_QAbstractVideoBuffer::mapModeDebug_data()
{
    QTest::addColumn<QAbstractVideoBuffer::MapMode>("mapMode");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(NotMapped);
    ADD_ENUM_TEST(ReadOnly);
    ADD_ENUM_TEST(WriteOnly);
    ADD_ENUM_TEST(ReadWrite);
}

void tst_QAbstractVideoBuffer::mapModeDebug()
{
    QFETCH(QAbstractVideoBuffer::MapMode, mapMode);
    QFETCH(QString, stringized);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << mapMode;
}

QTEST_MAIN(tst_QAbstractVideoBuffer)

#include "tst_qabstractvideobuffer.moc"
