/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite module of the Qt Toolkit.
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
#include <QtLocation/QPlaceSearchResult>
#include <QtLocation/QPlaceIcon>

QT_USE_NAMESPACE

class tst_QPlaceSearchResult : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructorTest();
    void title();
    void icon();
    void operators();
};

void tst_QPlaceSearchResult::constructorTest()
{
    QPlaceSearchResult result;

    QCOMPARE(result.type(), QPlaceSearchResult::UnknownSearchResult);
    QVERIFY(result.title().isEmpty());
    QVERIFY(result.icon().isEmpty());

    result.setTitle(QStringLiteral("title"));
    QPlaceIcon icon;
    QVariantMap parameters;
    parameters.insert(QStringLiteral("paramKey"), QStringLiteral("paramValue"));
    icon.setParameters(parameters);
    result.setIcon(icon);

    QPlaceSearchResult result2(result);
    QCOMPARE(result2.title(), QStringLiteral("title"));
    QCOMPARE(result2.icon().parameters().value(QStringLiteral("paramKey")).toString(),
             QStringLiteral("paramValue"));

    QCOMPARE(result2, result);
}

void tst_QPlaceSearchResult::title()
{
    QPlaceSearchResult result;
    QVERIFY(result.title().isEmpty());
    result.setTitle(QStringLiteral("title"));
    QCOMPARE(result.title(), QStringLiteral("title"));
    result.setTitle(QString());
    QVERIFY(result.title().isEmpty());
}

void tst_QPlaceSearchResult::icon()
{
    QPlaceSearchResult result;
    QVERIFY(result.icon().isEmpty());
    QPlaceIcon icon;
    QVariantMap iconParams;
    iconParams.insert(QStringLiteral("paramKey"), QStringLiteral("paramValue"));
    result.setIcon(icon);
    QCOMPARE(result.icon(), icon);
    result.setIcon(QPlaceIcon());
    QVERIFY(result.icon().isEmpty());
}

void tst_QPlaceSearchResult::operators()
{
    QPlaceSearchResult result1;
    QPlaceSearchResult result2;

    QVERIFY(result1 == result2);
    QVERIFY(!(result1 != result2));

    result1.setTitle(QStringLiteral("title"));
    QVERIFY(!(result1 == result2));
    QVERIFY(result1 != result2);

    result2.setTitle(QStringLiteral("title"));
    QVERIFY(result1 == result2);
    QVERIFY(!(result1 != result2));
}

QTEST_APPLESS_MAIN(tst_QPlaceSearchResult)

#include "tst_qplacesearchresult.moc"
