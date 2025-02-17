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


#include "qplatformdefs.h"
#include "qthreadonce.h"

#ifndef QT_NO_THREAD
#include "qmutex.h"

Q_GLOBAL_STATIC_WITH_ARGS(QMutex, onceInitializationMutex, (QMutex::Recursive))

enum QOnceExtra {
    MustRunCode = 0x01,
    MustUnlockMutex = 0x02
};

/*!
    \internal
    Initialize the Q_ONCE structure.

    Q_ONCE consists of two variables:
     - a static POD QOnceControl::ControlVariable (it's a QBasicAtomicInt)
     - an automatic QOnceControl that controls the former

    The POD is initialized to 0.

    When QOnceControl's constructor starts, it'll lock the global
    initialization mutex. It'll then check if it's the first to up
    the control variable and will take note.

    The QOnceControl's destructor will unlock the global
    initialization mutex.
*/
QOnceControl::QOnceControl(QBasicAtomicInt *control)
{
    d = 0;
    gv = control;
    // check if code has already run once
    if (gv->loadAcquire() == 2) {
        // uncontended case: it has already initialized
        // no waiting
        return;
    }

    // acquire the path
    onceInitializationMutex()->lock();
    extra = MustUnlockMutex;

    if (gv->testAndSetAcquire(0, 1)) {
        // path acquired, we're the first
        extra |= MustRunCode;
    }
}

QOnceControl::~QOnceControl()
{
    if (mustRunCode())
        // code wasn't run!
        gv->testAndSetRelease(1, 0);
    else
        gv->testAndSetRelease(1, 2);
    if (extra & MustUnlockMutex)
        onceInitializationMutex()->unlock();
}

/*!
    \internal
    Returns true if the initialization code must be run.

    Obviously, the initialization code must be run only once...
*/
bool QOnceControl::mustRunCode()
{
    return extra & MustRunCode;
}

void QOnceControl::done()
{
    extra &= ~MustRunCode;
}

#endif // QT_NO_THREAD
