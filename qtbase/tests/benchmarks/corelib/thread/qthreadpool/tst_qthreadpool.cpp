/****************************************************************************
**
** Copyright (C) 2013 David Faure <david.faure@kdab.com>
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

#include <qtest.h>
#include <QtCore>

class tst_QThreadPool : public QObject
{
    Q_OBJECT

public:
    tst_QThreadPool();
    ~tst_QThreadPool();

private slots:
    void startRunnables();
    void activeThreadCount();
};

tst_QThreadPool::tst_QThreadPool()
{
}

tst_QThreadPool::~tst_QThreadPool()
{
}

class NoOpRunnable : public QRunnable
{
public:
    void run() Q_DECL_OVERRIDE {
    }
};

void tst_QThreadPool::startRunnables()
{
    QThreadPool threadPool;
    threadPool.setMaxThreadCount(10);
    QBENCHMARK {
        threadPool.start(new NoOpRunnable());
    }
}

void tst_QThreadPool::activeThreadCount()
{
    QThreadPool threadPool;
    threadPool.start(new NoOpRunnable());
    QBENCHMARK {
        QVERIFY(threadPool.activeThreadCount() <= 10);
    }
}

QTEST_MAIN(tst_QThreadPool)
#include "tst_qthreadpool.moc"
