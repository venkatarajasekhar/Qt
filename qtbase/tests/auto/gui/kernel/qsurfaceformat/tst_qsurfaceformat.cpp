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

#include <qsurfaceformat.h>

#include <QtTest/QtTest>

class tst_QSurfaceFormat: public QObject
{
    Q_OBJECT

private slots:
    void versionCheck_data();
    void versionCheck();
};

void tst_QSurfaceFormat::versionCheck_data()
{
    QTest::addColumn<int>("formatMajor");
    QTest::addColumn<int>("formatMinor");
    QTest::addColumn<int>("compareMajor");
    QTest::addColumn<int>("compareMinor");
    QTest::addColumn<bool>("expected");

    QTest::newRow("lower major, lower minor")
        << 3 << 2 << 2 << 1 << true;
    QTest::newRow("lower major, same minor")
        << 3 << 2 << 2 << 2 << true;
    QTest::newRow("lower major, greater minor")
        << 3 << 2 << 2 << 3 << true;
    QTest::newRow("same major, lower minor")
        << 3 << 2 << 3 << 1 << true;
    QTest::newRow("same major, same minor")
        << 3 << 2 << 3 << 2 << true;
    QTest::newRow("same major, greater minor")
        << 3 << 2 << 3 << 3 << false;
    QTest::newRow("greater major, lower minor")
        << 3 << 2 << 4 << 1 << false;
    QTest::newRow("greater major, same minor")
        << 3 << 2 << 4 << 2 << false;
    QTest::newRow("greater major, greater minor")
        << 3 << 2 << 4 << 3 << false;
}

void tst_QSurfaceFormat::versionCheck()
{
    QFETCH( int, formatMajor );
    QFETCH( int, formatMinor );
    QFETCH( int, compareMajor );
    QFETCH( int, compareMinor );
    QFETCH( bool, expected );

    QSurfaceFormat format;
    format.setMinorVersion(formatMinor);
    format.setMajorVersion(formatMajor);

    QCOMPARE(format.version() >= qMakePair(compareMajor, compareMinor), expected);

    format.setVersion(formatMajor, formatMinor);
    QCOMPARE(format.version() >= qMakePair(compareMajor, compareMinor), expected);
}

#include <tst_qsurfaceformat.moc>
QTEST_MAIN(tst_QSurfaceFormat);
