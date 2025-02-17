/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

/*!
    \class QRunnable
    \inmodule QtCore
    \since 4.4
    \brief The QRunnable class is the base class for all runnable objects.

    \ingroup thread

    The QRunnable class is an interface for representing a task or
    piece of code that needs to be executed, represented by your
    reimplementation of the run() function.

    You can use QThreadPool to execute your code in a separate
    thread. QThreadPool deletes the QRunnable automatically if
    autoDelete() returns \c true (the default). Use setAutoDelete() to
    change the auto-deletion flag.

    QThreadPool supports executing the same QRunnable more than once
    by calling QThreadPool::tryStart(this) from within the run() function.
    If autoDelete is enabled the QRunnable will be deleted when
    the last thread exits the run function. Calling QThreadPool::start()
    multiple times with the same QRunnable when autoDelete is enabled
    creates a race condition and is not recommended.

    \sa QThreadPool
*/

/*! \fn QRunnable::run()
    Implement this pure virtual function in your subclass.
*/

/*! \fn QRunnable::QRunnable()
    Constructs a QRunnable. Auto-deletion is enabled by default.

    \sa autoDelete(), setAutoDelete()
*/

/*! \fn QRunnable::~QRunnable()
    QRunnable virtual destructor.
*/

/*! \fn bool QRunnable::autoDelete() const

    Returns \c true is auto-deletion is enabled; false otherwise.

    If auto-deletion is enabled, QThreadPool will automatically delete
    this runnable after calling run(); otherwise, ownership remains
    with the application programmer.

    \sa setAutoDelete(), QThreadPool
*/

/*! \fn bool QRunnable::setAutoDelete(bool autoDelete)

    Enables auto-deletion if \a autoDelete is true; otherwise
    auto-deletion is disabled.

    If auto-deletion is enabled, QThreadPool will automatically delete
    this runnable after calling run(); otherwise, ownership remains
    with the application programmer.

    Note that this flag must be set before calling
    QThreadPool::start(). Calling this function after
    QThreadPool::start() results in undefined behavior.

    \sa autoDelete(), QThreadPool
*/
