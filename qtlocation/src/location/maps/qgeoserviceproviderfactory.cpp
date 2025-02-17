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

#include "qgeoserviceproviderfactory.h"

QT_BEGIN_NAMESPACE

/*!
    \class QGeoServiceProviderFactory
    \inmodule QtLocation
    \ingroup QtLocation-impl
    \since Qt Location 5.0

    \brief The QGeoServiceProviderFactory class is a factory class used as the
    plugin interface for services related to geographical information.

    Implementers must provide a unique combination of providerName() and
    providerVersion() per plugin.

    The other functions should be overridden if the plugin supports the
    associated set of functionality.
*/

/*!
\fn QGeoServiceProviderFactory::~QGeoServiceProviderFactory()

Destroys this QGeoServiceProviderFactory instance.
*/

/*!
    Returns a new QGeoCodingManagerEngine instance, initialized with \a
    parameters, which implements the location geocoding functionality.

    If \a error is not 0 it should be set to QGeoServiceProvider::NoError on
    success or an appropriate QGeoServiceProvider::Error on failure.

    If \a errorString is not 0 it should be set to a string describing any
    error which occurred.

    The default implementation returns 0, which causes a
    QGeoServiceProvider::NotSupportedError in QGeoServiceProvider.
*/
QGeoCodingManagerEngine *QGeoServiceProviderFactory::createGeocodingManagerEngine(const QVariantMap &parameters,
                                                                                  QGeoServiceProvider::Error *error,
                                                                                  QString *errorString) const
{
    Q_UNUSED(parameters)
    Q_UNUSED(error)
    Q_UNUSED(errorString)

    return 0;
}

/*!
    Returns a new QGeoMappingManagerEngine instance, initialized with \a
    parameters, which implements mapping functionality.

    If \a error is not 0 it should be set to QGeoServiceProvider::NoError on
    success or an appropriate QGeoServiceProvider::Error on failure.

    If \a errorString is not 0 it should be set to a string describing any
    error which occurred.

    The default implementation returns 0, which causes a
    QGeoServiceProvider::NotSupportedError in QGeoServiceProvider.

    \internal
*/
QGeoMappingManagerEngine *QGeoServiceProviderFactory::createMappingManagerEngine(const QVariantMap &parameters,
                                                                                 QGeoServiceProvider::Error *error,
                                                                                 QString *errorString) const
{
    Q_UNUSED(parameters)
    Q_UNUSED(error)
    Q_UNUSED(errorString)

    return 0;
}

/*!
    Returns a new QGeoRoutingManagerEngine instance, initialized with \a
    parameters, which implements routing functionality.

    If \a error is not 0 it should be set to QGeoServiceProvider::NoError on
    success or an appropriate QGeoServiceProvider::Error on failure.

    If \a errorString is not 0 it should be set to a string describing any
    error which occurred.

    The default implementation returns 0, which causes a
    QGeoServiceProvider::NotSupportedError in QGeoServiceProvider.
*/
QGeoRoutingManagerEngine *QGeoServiceProviderFactory::createRoutingManagerEngine(const QVariantMap &parameters,
                                                                                 QGeoServiceProvider::Error *error,
                                                                                 QString *errorString) const

{
    Q_UNUSED(parameters)
    Q_UNUSED(error)
    Q_UNUSED(errorString)

    return 0;
}

/*!
    Returns a new QPlaceManagerEngine instance, initialized with \a
    parameters, which implements the place searching functionality.

    If \a error is not 0 it should be set to QGeoServiceProvider::NoError on
    success or an appropriate QGeoServiceProvider::Error on failure.

    If \a errorString is not 0 it should be set to a string describing any
    error which occurred.

    The default implementation returns 0, which causes a
    QGeoServiceProvider::NotSupportedError in QGeoServiceProvider.
*/
QPlaceManagerEngine *QGeoServiceProviderFactory::createPlaceManagerEngine(const QVariantMap &parameters,
                                                                          QGeoServiceProvider::Error *error,
                                                                          QString *errorString) const

{
    Q_UNUSED(parameters)
    Q_UNUSED(error)
    Q_UNUSED(errorString)

    return 0;
}

QT_END_NAMESPACE
