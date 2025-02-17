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

#include <QtCore>
#include <QtTest/QtTest>

#define INVOKE_COUNT 10000

class qtimer_vs_qmetaobject : public QObject
{
    Q_OBJECT
private slots:
    void testZeroTimerSingleShot();
    void testQueuedInvokeMethod();
};

class InvokeCounter : public QObject {
    Q_OBJECT
public:
    InvokeCounter() : count(0) { };
public slots:
    void invokeSlot() {
        count++;
        if (count == INVOKE_COUNT)
            QTestEventLoop::instance().exitLoop();
    }
protected:
    int count;
};

void qtimer_vs_qmetaobject::testZeroTimerSingleShot()
{
    QBENCHMARK {
        InvokeCounter invokeCounter;
        for(int i = 0; i < INVOKE_COUNT; ++i) {
            QTimer::singleShot(0, &invokeCounter, SLOT(invokeSlot()));
        }
        QTestEventLoop::instance().enterLoop(10);
        QVERIFY(!QTestEventLoop::instance().timeout());
    }
}

void qtimer_vs_qmetaobject::testQueuedInvokeMethod()
{
    QBENCHMARK {
        InvokeCounter invokeCounter;
        for(int i = 0; i < INVOKE_COUNT; ++i) {
            QMetaObject::invokeMethod(&invokeCounter, "invokeSlot", Qt::QueuedConnection);
        }
        QTestEventLoop::instance().enterLoop(10);
        QVERIFY(!QTestEventLoop::instance().timeout());
    }
}


QTEST_MAIN(qtimer_vs_qmetaobject)

#include "tst_qtimer_vs_qmetaobject.moc"
