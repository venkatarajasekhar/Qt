/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtLocation module of the Qt Toolkit.
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

#include "qplacecontentreply.h"
#include "qplacereply_p.h"

QT_BEGIN_NAMESPACE
class QPlaceContentReplyPrivate : public QPlaceReplyPrivate
{
public:
    QPlaceContentReplyPrivate()
    :   totalCount(0)
    { }

    QPlaceContent::Collection contentCollection;
    int totalCount;
    QPlaceContentRequest contentRequest;
    QPlaceContentRequest previousPageRequest;
    QPlaceContentRequest nextPageRequest;
};

QT_END_NAMESPACE

QT_USE_NAMESPACE

/*!
    \class QPlaceContentReply
    \inmodule QtLocation
    \ingroup QtLocation-places
    \ingroup QtLocation-places-replies
    \since Qt Location 5.0

    \brief The QPlaceContentReply class manages a content retrieval operation started by an
    instance of QPlaceManager.

    See \l {Fetching Rich Content} for an example on how to use a content reply.
    \sa QPlaceContentRequest, QPlaceManager
*/

/*!
    Constructs a content reply with a given \a parent.
*/
QPlaceContentReply::QPlaceContentReply(QObject *parent)
    : QPlaceReply(new QPlaceContentReplyPrivate, parent)
{
}

/*!
    Destroys the reply.
*/
QPlaceContentReply::~QPlaceContentReply()
{
}

 /*!
    Returns the collection of content retrieved.
*/
QPlaceContent::Collection QPlaceContentReply::content() const
{
    Q_D(const QPlaceContentReply);
    return d->contentCollection;
}

/*!
    Returns the type of reply.
*/
QPlaceReply::Type QPlaceContentReply::type() const
{
    return QPlaceReply::ContentReply;
}

/*!
    Sets the \a content of the reply.
*/
void QPlaceContentReply::setContent(const QPlaceContent::Collection &content)
{
    Q_D(QPlaceContentReply);
    d->contentCollection = content;
}

/*!
    Returns the total number of content objects for a place.  If the total number of
    content objects cannot be counted, a value of -1 is returned.  This count only
    refers to the total count for a single content type, that is, the content type that
    was specified when content was requested with the QPlaceManager.
*/
int QPlaceContentReply::totalCount() const
{
    Q_D(const QPlaceContentReply);
    return d->totalCount;
}

/*!
    Sets the \a total number of content objects for a place.
*/
void QPlaceContentReply::setTotalCount(int total)
{
    Q_D(QPlaceContentReply);
    d->totalCount = total;
}

/*!
    Returns the content request that was used to generate this reply.
*/
QPlaceContentRequest QPlaceContentReply::request() const
{
    Q_D(const QPlaceContentReply);
    return d->contentRequest;
}

/*!
    Returns a place content request that can be used to request the previous batch of place content
    results.
*/
QPlaceContentRequest QPlaceContentReply::previousPageRequest() const
{
    Q_D(const QPlaceContentReply);
    return d->previousPageRequest;
}

/*!
    Returns a place content request that can be used to request the next batch of place content
    results.
*/
QPlaceContentRequest QPlaceContentReply::nextPageRequest() const
{
    Q_D(const QPlaceContentReply);
    return d->nextPageRequest;
}

/*!
    Sets the content \a request used to generate this this reply.
*/
void QPlaceContentReply::setRequest(const QPlaceContentRequest &request)
{
    Q_D(QPlaceContentReply);
    d->contentRequest = request;
}

/*!
    Sets the place content request that can be used to request the previous batch of place content
    results to \a previous.
*/
void QPlaceContentReply::setPreviousPageRequest(const QPlaceContentRequest &previous)
{
    Q_D(QPlaceContentReply);
    d->previousPageRequest = previous;
}

/*!
    Sets the place content request that can be used to request the next batch of place content
    results to \a next.
*/
void QPlaceContentReply::setNextPageRequest(const QPlaceContentRequest &next)
{
    Q_D(QPlaceContentReply);
    d->nextPageRequest = next;
}
