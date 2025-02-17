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

#include <QtCore/QString>
#include <QtTest/QtTest>

#include <qplaceratings.h>

QT_USE_NAMESPACE

class tst_QPlaceRatings : public QObject
{
    Q_OBJECT

public:
    tst_QPlaceRatings();

private Q_SLOTS:
    void constructorTest();
    void averageTest();
    void countTest();
    void operatorsTest();
    void isEmptyTest();
};

tst_QPlaceRatings::tst_QPlaceRatings()
{
}

void tst_QPlaceRatings::constructorTest()
{
    QPlaceRatings testObj;
    Q_UNUSED(testObj);

    QPlaceRatings *testObjPtr = new QPlaceRatings(testObj);
    QVERIFY2(testObjPtr != NULL, "Copy constructor - null");
    QVERIFY2(testObjPtr->count() == 0, "Copy constructor - wrong count");
    QVERIFY2(testObjPtr->average() == 0, "Copy constructor - wrong average");
    QVERIFY2(*testObjPtr == testObj, "Copy constructor - compare");
    delete testObjPtr;
}

void tst_QPlaceRatings::averageTest()
{
    QPlaceRatings testObj;
    QVERIFY2(qFuzzyCompare(testObj.average(), 0) , "Wrong default average");
    testObj.setAverage(-10.23);
    QCOMPARE(testObj.average(), -10.23);
}

void tst_QPlaceRatings::countTest()
{
    QPlaceRatings testObj;
    QVERIFY2(testObj.count() == 0, "Wrong default value");
    testObj.setCount(-1002);
    QVERIFY2(testObj.count() == -1002, "Wrong value returned");
}

void tst_QPlaceRatings::operatorsTest()
{
    QPlaceRatings testObj;
    testObj.setAverage(0.123);
    QPlaceRatings testObj2;
    testObj2 = testObj;
    QVERIFY2(testObj == testObj2, "Not copied correctly");
    testObj2.setCount(-10);
    QVERIFY2(testObj != testObj2, "Object should be different");
}

void tst_QPlaceRatings::isEmptyTest()
{
    QPlaceRatings ratings;

    QVERIFY(ratings.isEmpty());

    ratings.setCount(1);
    QVERIFY(!ratings.isEmpty());
    ratings.setCount(0);
    QVERIFY(ratings.isEmpty());

    ratings.setMaximum(1);
    QVERIFY(!ratings.isEmpty());
    ratings.setMaximum(0);
    QVERIFY(ratings.isEmpty());

    ratings.setAverage(1);
    QVERIFY(!ratings.isEmpty());
    ratings.setAverage(0);
    QVERIFY(ratings.isEmpty());
}

QTEST_APPLESS_MAIN(tst_QPlaceRatings);

#include "tst_qplaceratings.moc"
