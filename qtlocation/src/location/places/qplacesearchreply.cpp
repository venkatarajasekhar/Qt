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

#include <QtLocation/QPlaceSearchRequest>
#include <QtLocation/QPlaceSearchReply>
#include <QtLocation/QPlaceProposedSearchResult>
#include <QtLocation/private/qplacereply_p.h>

QT_BEGIN_NAMESPACE

class QPlaceSearchReplyPrivate : public QPlaceReplyPrivate
{
public:
    QPlaceSearchReplyPrivate(){}
    QList<QPlaceSearchResult> results;
    QPlaceSearchRequest searchRequest;
    QPlaceSearchRequest previousPageRequest;
    QPlaceSearchRequest nextPageRequest;
};

/*!
    \class QPlaceSearchReply
    \inmodule QtLocation
    \ingroup QtLocation-places
    \ingroup QtLocation-places-replies
    \since Qt Location 5.0

    \brief The QPlaceSearchReply class manages a place search operation started by an
    instance of QPlaceManager.

    See \l {Discovery/Search} for an example on how to use a search reply.
    \sa QPlaceSearchRequest, QPlaceManager
*/

/*!
    Constructs a search reply with a given \a parent.
*/
QPlaceSearchReply::QPlaceSearchReply(QObject *parent)
    : QPlaceReply(new QPlaceSearchReplyPrivate, parent)
{
}

/*!
    Destroys the search reply.
*/
QPlaceSearchReply::~QPlaceSearchReply()
{
}

/*!
    Returns the type of reply.
*/
QPlaceReply::Type QPlaceSearchReply::type() const
{
    return QPlaceReply::SearchReply;
}

 /*!
    Returns a list of search results;
*/
QList<QPlaceSearchResult> QPlaceSearchReply::results() const
{
    Q_D(const QPlaceSearchReply);
    return d->results;
}

/*!
    Sets the list of search \a results.
*/
void QPlaceSearchReply::setResults(const QList<QPlaceSearchResult> &results)
{
    Q_D(QPlaceSearchReply);
    d->results = results;
}

/*!
    Returns the search request that was used to generate this reply.
*/
QPlaceSearchRequest QPlaceSearchReply::request() const
{
    Q_D(const QPlaceSearchReply);
    return d->searchRequest;
}

/*!
    Returns a place search request which can be used to request the previous page of search
    results. An empty place search request is returned if there is no previous page of results.

    \sa nextPageRequest(), setPreviousPageRequest()
*/
QPlaceSearchRequest QPlaceSearchReply::previousPageRequest() const
{
    Q_D(const QPlaceSearchReply);
    return d->previousPageRequest;
}

/*!
    Returns a place search request which can be used to request the next page of search results. An
    empty place search request is returned if there is no next page of results.

    \sa previousPageRequest(), setNextPageRequest()
*/
QPlaceSearchRequest QPlaceSearchReply::nextPageRequest() const
{
    Q_D(const QPlaceSearchReply);
    return d->nextPageRequest;
}

/*!
    Sets the search \a request used to generate this reply.
*/
void QPlaceSearchReply::setRequest(const QPlaceSearchRequest &request)
{
    Q_D(QPlaceSearchReply);
    d->searchRequest = request;
}

/*!
    Sets the previous page of search results request to \a previous.

    \sa previousPageRequest()
*/
void QPlaceSearchReply::setPreviousPageRequest(const QPlaceSearchRequest &previous)
{
    Q_D(QPlaceSearchReply);
    d->previousPageRequest = previous;
}

/*!
    Sets the next page of search results request to \a next.

    \sa nextPageRequest()
*/
void QPlaceSearchReply::setNextPageRequest(const QPlaceSearchRequest &next)
{
    Q_D(QPlaceSearchReply);
    d->nextPageRequest = next;
}

QT_END_NAMESPACE
