/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
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

#include <QtCore/qmetaobject.h>
#include <QtCore/qtimer.h>

#include "qmediacontrol.h"
#include "qmediacontrol_p.h"



QT_BEGIN_NAMESPACE

/*!
    \class QMediaControl
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_control
    \ingroup multimedia_core

    \brief The QMediaControl class provides a base interface for media service controls.

    Media controls provide an interface to individual features provided by a
    media service.  Most services implement a principal control which exposes
    the core functionality of the service and a number of optional controls which
    expose any additional functionality.

    A pointer to a control implemented by a media service can be obtained using
    the \l {QMediaService::requestControl()} member of QMediaService.  If the
    service doesn't implement a control it will instead return a null pointer.

    \snippet multimedia-snippets/media.cpp Request control

    Alternatively if the IId of the control has been declared using
    Q_MEDIA_DECLARE_CONTROL the template version of
    QMediaService::requestControl() can be used to request the service without
    explicitly passing the IId or using qobject_cast().

    \snippet multimedia-snippets/media.cpp Request control templated

    Most application code will not interface directly with a media service's
    controls, instead the QMediaObject which owns the service acts as an
    intermediary between one or more controls and the application.

    \sa QMediaService, QMediaObject
*/

/*!
    \macro Q_MEDIA_DECLARE_CONTROL(Class, IId)
    \relates QMediaControl

    The Q_MEDIA_DECLARE_CONTROL macro declares an \a IId for a \a Class that
    inherits from QMediaControl.

    Declaring an IId for a QMediaControl allows an instance of that control to
    be requested from QMediaService::requestControl() without explicitly
    passing the IId.

    \snippet multimedia-snippets/media.cpp Request control templated

    \sa QMediaService::requestControl()
*/

/*!
    Destroys a media control.
*/

QMediaControl::~QMediaControl()
{
    delete d_ptr;
}

/*!
    Constructs a media control with the given \a parent.
*/

QMediaControl::QMediaControl(QObject *parent)
    : QObject(parent)
    , d_ptr(new QMediaControlPrivate)
{
    d_ptr->q_ptr = this;
}

/*!
    \internal
*/

QMediaControl::QMediaControl(QMediaControlPrivate &dd, QObject *parent)
    : QObject(parent)
    , d_ptr(&dd)

{
    d_ptr->q_ptr = this;
}

#include "moc_qmediacontrol.cpp"
QT_END_NAMESPACE

