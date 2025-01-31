/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Alex Char.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the ICNS autotests in the Qt ImageFormats module.
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

#include <QtTest/QtTest>
#include <QtGui/QtGui>

class tst_qicns: public QObject
{
    Q_OBJECT

private slots:
    void readIcons_data();
    void readIcons();
    void writeIcons_data();
    void writeIcons();
};

void tst_qicns::readIcons_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<int>("imageCount");
    QTest::addColumn<QByteArray>("format");

    QTest::newRow("1") << QStringLiteral("test-png") << QSize(128, 128) << 7 << QByteArrayLiteral("png");
    QTest::newRow("2") << QStringLiteral("test-jp2") << QSize(128, 128) << 7 << QByteArrayLiteral("jp2");
    QTest::newRow("3") << QStringLiteral("test-32bit") << QSize(128, 128) << 4 << QByteArray();
    QTest::newRow("4") << QStringLiteral("test-legacy") << QSize(48, 48) << 12 << QByteArray();
    QTest::newRow("5") << QStringLiteral("test-variants") << QSize(128, 128) << 5 << QByteArrayLiteral("png");
}

void tst_qicns::readIcons()
{
    QFETCH(QString, fileName);
    QFETCH(QSize, size);
    QFETCH(int, imageCount);
    QFETCH(QByteArray, format);

    if (!format.isEmpty() && !QImageReader::supportedImageFormats().contains(format))
        QSKIP("This test requires another image format plugin");
    const QString path = QStringLiteral(":/icns/") + fileName + QStringLiteral(".icns");
    QImageReader reader(path);
    QVERIFY(reader.canRead());
    QCOMPARE(reader.imageCount(), imageCount);

    for (int i = 0; i < reader.imageCount(); ++i) {
        QVERIFY2(reader.jumpToImage(i), qPrintable(reader.errorString()));
        QImage image = reader.read();
        if (i == 0)
            QCOMPARE(image.size(), size);
        QVERIFY2(!image.isNull(), qPrintable(reader.errorString()));
    }
}

void tst_qicns::writeIcons_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QSize>("size");

    QTest::newRow("1") << QStringLiteral("test-write-16") << QSize(16, 16);
    QTest::newRow("2") << QStringLiteral("test-write-32") << QSize(32, 32);
    QTest::newRow("3") << QStringLiteral("test-write-64") << QSize(64, 64);
    QTest::newRow("4") << QStringLiteral("test-write-128") << QSize(128, 128);
    QTest::newRow("5") << QStringLiteral("test-write-512") << QSize(512, 512);
    QTest::newRow("6") << QStringLiteral("test-write-1024") << QSize(1024, 1024);
}

void tst_qicns::writeIcons()
{
    QTemporaryDir temp(QDir::tempPath() + QStringLiteral("/tst_qincs"));
    QVERIFY2(temp.isValid(), "Unable to create temp dir.");

    QFETCH(QString, fileName);
    QFETCH(QSize, size);

    const QString distPath = QStringLiteral("%1/%2.icns").arg(temp.path()).arg(fileName);
    const QString sourcePath = QStringLiteral(":/icns/%1.png").arg(fileName);

    QImage image(sourcePath);
    QVERIFY(!image.isNull());
    QVERIFY(image.size() == size);

    QImageWriter writer(distPath, QByteArrayLiteral("icns"));
    QVERIFY2(writer.canWrite(), qPrintable(writer.errorString()));
    QVERIFY2(writer.write(image), qPrintable(writer.errorString()));

    QVERIFY(image == QImage(distPath));
}

QTEST_MAIN(tst_qicns)
#include "tst_qicns.moc"
