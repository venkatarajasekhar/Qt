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

#include "qsignaltransition.h"

#ifndef QT_NO_STATEMACHINE

#include "qsignaltransition_p.h"
#include "qstate.h"
#include "qstate_p.h"
#include "qstatemachine.h"
#include "qstatemachine_p.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*!
  \class QSignalTransition
  \inmodule QtCore

  \brief The QSignalTransition class provides a transition based on a Qt signal.

  \since 4.6
  \ingroup statemachine

  Typically you would use the overload of QState::addTransition() that takes a
  sender and signal as arguments, rather than creating QSignalTransition
  objects directly. QSignalTransition is part of \l{The State Machine
  Framework}.

  You can subclass QSignalTransition and reimplement eventTest() to make a
  signal transition conditional; the event object passed to eventTest() will
  be a QStateMachine::SignalEvent object. Example:

  \code
  class CheckedTransition : public QSignalTransition
  {
  public:
      CheckedTransition(QCheckBox *check)
          : QSignalTransition(check, SIGNAL(stateChanged(int))) {}
  protected:
      bool eventTest(QEvent *e) {
          if (!QSignalTransition::eventTest(e))
              return false;
          QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(e);
          return (se->arguments().at(0).toInt() == Qt::Checked);
      }
  };

  ...

  QCheckBox *check = new QCheckBox();
  check->setTristate(true);

  QState *s1 = new QState();
  QState *s2 = new QState();
  CheckedTransition *t1 = new CheckedTransition(check);
  t1->setTargetState(s2);
  s1->addTransition(t1);
  \endcode
*/

/*!
    \property QSignalTransition::senderObject

    \brief the sender object that this signal transition is associated with
*/

/*!
    \property QSignalTransition::signal

    \brief the signal that this signal transition is associated with
*/

QSignalTransitionPrivate::QSignalTransitionPrivate()
{
    sender = 0;
    signalIndex = -1;
}

QSignalTransitionPrivate *QSignalTransitionPrivate::get(QSignalTransition *q)
{
    return q->d_func();
}

void QSignalTransitionPrivate::unregister()
{
    Q_Q(QSignalTransition);
    if ((signalIndex == -1) || !machine())
        return;
    QStateMachinePrivate::get(machine())->unregisterSignalTransition(q);
}

void QSignalTransitionPrivate::maybeRegister()
{
    Q_Q(QSignalTransition);
    if (QStateMachine *mach = machine())
        QStateMachinePrivate::get(mach)->maybeRegisterSignalTransition(q);
}

/*!
  Constructs a new signal transition with the given \a sourceState.
*/
QSignalTransition::QSignalTransition(QState *sourceState)
    : QAbstractTransition(*new QSignalTransitionPrivate, sourceState)
{
}

/*!
  Constructs a new signal transition associated with the given \a signal of
  the given \a sender, and with the given \a sourceState.
*/
QSignalTransition::QSignalTransition(const QObject *sender, const char *signal,
                                     QState *sourceState)
    : QAbstractTransition(*new QSignalTransitionPrivate, sourceState)
{
    Q_D(QSignalTransition);
    d->sender = sender;
    d->signal = signal;
    d->maybeRegister();
}

/*!
  Destroys this signal transition.
*/
QSignalTransition::~QSignalTransition()
{
}

/*!
  Returns the sender object associated with this signal transition.
*/
QObject *QSignalTransition::senderObject() const
{
    Q_D(const QSignalTransition);
    return const_cast<QObject *>(d->sender);
}

/*!
  Sets the \a sender object associated with this signal transition.
*/
void QSignalTransition::setSenderObject(const QObject *sender)
{
    Q_D(QSignalTransition);
    if (sender == d->sender)
        return;
    d->unregister();
    d->sender = sender;
    d->maybeRegister();
    emit senderObjectChanged(QPrivateSignal());
}

/*!
  Returns the signal associated with this signal transition.
*/
QByteArray QSignalTransition::signal() const
{
    Q_D(const QSignalTransition);
    return d->signal;
}

/*!
  Sets the \a signal associated with this signal transition.
*/
void QSignalTransition::setSignal(const QByteArray &signal)
{
    Q_D(QSignalTransition);
    if (signal == d->signal)
        return;
    d->unregister();
    d->signal = signal;
    d->maybeRegister();
    emit signalChanged(QPrivateSignal());
}

/*!
  \reimp

  The default implementation returns \c true if the \a event is a
  QStateMachine::SignalEvent object and the event's sender and signal index
  match this transition, and returns \c false otherwise.
*/
bool QSignalTransition::eventTest(QEvent *event)
{
    Q_D(const QSignalTransition);
    if (event->type() == QEvent::StateMachineSignal) {
        if (d->signalIndex == -1)
            return false;
        QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(event);
        return (se->sender() == d->sender)
            && (se->signalIndex() == d->signalIndex);
    }
    return false;
}

/*!
  \reimp
*/
void QSignalTransition::onTransition(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
  \reimp
*/
bool QSignalTransition::event(QEvent *e)
{
    return QAbstractTransition::event(e);
}

/*!
  \fn QSignalTransition::senderObjectChanged()
  \since 5.4

  This signal is emitted when the senderObject property is changed.

  \sa QSignalTransition::senderObject
*/

/*!
  \fn QSignalTransition::signalChanged()
  \since 5.4

  This signal is emitted when the signal property is changed.

  \sa QSignalTransition::signal
*/

void QSignalTransitionPrivate::callOnTransition(QEvent *e)
{
    Q_Q(QSignalTransition);

    if (e->type() == QEvent::StateMachineSignal) {
        QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent *>(e);
        int savedSignalIndex = se->m_signalIndex;
        se->m_signalIndex = originalSignalIndex;
        q->onTransition(e);
        se->m_signalIndex = savedSignalIndex;
    } else {
        q->onTransition(e);
    }
}


QT_END_NAMESPACE

#endif //QT_NO_STATEMACHINE
