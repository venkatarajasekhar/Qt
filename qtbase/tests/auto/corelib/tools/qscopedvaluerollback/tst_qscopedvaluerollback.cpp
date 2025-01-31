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

#include <QtTest/QtTest>
#include <QtCore/QScopedValueRollback>

/*!
 \class tst_QScopedValueRollback
 \internal
 \since 4.8
 \brief Tests class QScopedValueRollback.

 */
class tst_QScopedValueRollback : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void leavingScope();
    void leavingScopeAfterCommit();
    void rollbackToPreviousCommit();
    void exceptions();
    void earlyExitScope();
private:
    void earlyExitScope_helper(int exitpoint, int &member);
};

void tst_QScopedValueRollback::leavingScope()
{
    int i = 0;
    bool b = false;
    bool b2 = false;
    QString s("This is useful");

    //test rollback on going out of scope
    {
        QScopedValueRollback<int> ri(i);
        QScopedValueRollback<bool> rb(b);
        QScopedValueRollback<bool> rb2(b2, true);
        QScopedValueRollback<QString> rs(s);
        QCOMPARE(b, false);
        QCOMPARE(b2, true);
        QCOMPARE(i, 0);
        QCOMPARE(s, QString("This is useful"));
        b = true;
        i = 1;
        s = "Useless";
        QCOMPARE(b, true);
        QCOMPARE(i, 1);
        QCOMPARE(s, QString("Useless"));
    }
    QCOMPARE(b, false);
    QCOMPARE(b2, false);
    QCOMPARE(i, 0);
    QCOMPARE(s, QString("This is useful"));
}

void tst_QScopedValueRollback::leavingScopeAfterCommit()
{
    int i = 0;
    bool b = false;
    QString s("This is useful");

    //test rollback on going out of scope
    {
        QScopedValueRollback<int> ri(i);
        QScopedValueRollback<bool> rb(b);
        QScopedValueRollback<QString> rs(s);
        QCOMPARE(b, false);
        QCOMPARE(i, 0);
        QCOMPARE(s, QString("This is useful"));
        b = true;
        i = 1;
        s = "Useless";
        QCOMPARE(b, true);
        QCOMPARE(i, 1);
        QCOMPARE(s, QString("Useless"));
        ri.commit();
        rb.commit();
        rs.commit();
    }
    QCOMPARE(b, true);
    QCOMPARE(i, 1);
    QCOMPARE(s, QString("Useless"));
}

void tst_QScopedValueRollback::rollbackToPreviousCommit()
{
    int i=0;
    {
        QScopedValueRollback<int> ri(i);
        i++;
        ri.commit();
        i++;
    }
    QCOMPARE(i,1);
    {
        QScopedValueRollback<int> ri1(i);
        i++;
        ri1.commit();
        i++;
        ri1.commit();
        i++;
    }
    QCOMPARE(i,3);
}

void tst_QScopedValueRollback::exceptions()
{
    bool b = false;
    bool caught = false;
    QT_TRY
    {
        QScopedValueRollback<bool> rb(b);
        b = true;
        QT_THROW(std::bad_alloc()); //if Qt compiled without exceptions this is noop
        rb.commit(); //if Qt compiled without exceptions, true is committed
    }
    QT_CATCH(...)
    {
        caught = true;
    }
    QCOMPARE(b, !caught); //expect false if exception was thrown, true otherwise
}

void tst_QScopedValueRollback::earlyExitScope()
{
    int i=0;
    int j=0;
    while (true) {
        QScopedValueRollback<int> ri(i);
        i++;
        j=i;
        if (i>8) break;
        ri.commit();
    }
    QCOMPARE(i,8);
    QCOMPARE(j,9);

    for (i = 0; i < 5; i++) {
        j=1;
        earlyExitScope_helper(i,j);
        QCOMPARE(j, 1<<i);
    }
}

void tst_QScopedValueRollback::earlyExitScope_helper(int exitpoint, int& member)
{
    QScopedValueRollback<int> r(member);
    member *= 2;
    if (exitpoint == 0)
        return;
    r.commit();
    member *= 2;
    if (exitpoint == 1)
        return;
    r.commit();
    member *= 2;
    if (exitpoint == 2)
        return;
    r.commit();
    member *= 2;
    if (exitpoint == 3)
        return;
    r.commit();
}

QTEST_MAIN(tst_QScopedValueRollback)
#include "tst_qscopedvaluerollback.moc"
