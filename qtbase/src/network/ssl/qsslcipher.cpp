/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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
    \class QSslCipher
    \brief The QSslCipher class represents an SSL cryptographic cipher.
    \since 4.3

    \reentrant
    \ingroup network
    \ingroup ssl
    \ingroup shared
    \inmodule QtNetwork

    QSslCipher stores information about one cryptographic cipher. It
    is most commonly used with QSslSocket, either for configuring
    which ciphers the socket can use, or for displaying the socket's
    ciphers to the user.

    \sa QSslSocket, QSslKey
*/

#include "qsslcipher.h"
#include "qsslcipher_p.h"
#include "qsslsocket.h"

#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    Constructs an empty QSslCipher object.
*/
QSslCipher::QSslCipher()
    : d(new QSslCipherPrivate)
{
}

/*!
    Constructs a QSslCipher object for the cipher determined by \a
    name. The constructor accepts only supported ciphers (i.e., the
    \a name must identify a cipher in the list of ciphers returned by
    QSslSocket::supportedCiphers()).

    You can call isNull() after construction to check if \a name
    correctly identified a supported cipher.
*/
QSslCipher::QSslCipher(const QString &name)
    : d(new QSslCipherPrivate)
{
    foreach (const QSslCipher &cipher, QSslSocket::supportedCiphers()) {
        if (cipher.name() == name) {
            *this = cipher;
            return;
        }
    }
}

/*!
    Constructs a QSslCipher object for the cipher determined by \a
    name and \a protocol. The constructor accepts only supported
    ciphers (i.e., the \a name and \a protocol must identify a cipher
    in the list of ciphers returned by
    QSslSocket::supportedCiphers()).

    You can call isNull() after construction to check if \a name and
    \a protocol correctly identified a supported cipher.
*/
QSslCipher::QSslCipher(const QString &name, QSsl::SslProtocol protocol)
    : d(new QSslCipherPrivate)
{
    foreach (const QSslCipher &cipher, QSslSocket::supportedCiphers()) {
        if (cipher.name() == name && cipher.protocol() == protocol) {
            *this = cipher;
            return;
        }
    }
}

/*!
    Constructs an identical copy of the \a other cipher.
*/
QSslCipher::QSslCipher(const QSslCipher &other)
    : d(new QSslCipherPrivate)
{
    *d.data() = *other.d.data();
}

/*!
    Destroys the QSslCipher object.
*/
QSslCipher::~QSslCipher()
{
}

/*!
    Copies the contents of \a other into this cipher, making the two
    ciphers identical.
*/
QSslCipher &QSslCipher::operator=(const QSslCipher &other)
{
    *d.data() = *other.d.data();
    return *this;
}

/*!
    \fn void QSslCipher::swap(QSslCipher &other)
    \since 5.0

    Swaps this cipher instance with \a other. This function is very
    fast and never fails.
*/

/*!
    Returns \c true if this cipher is the same as \a other; otherwise,
    false is returned.
*/
bool QSslCipher::operator==(const QSslCipher &other) const
{
    return d->name == other.d->name && d->protocol == other.d->protocol;
}

/*!
    \fn bool QSslCipher::operator!=(const QSslCipher &other) const

    Returns \c true if this cipher is not the same as \a other;
    otherwise, false is returned.
*/

/*!
    Returns \c true if this is a null cipher; otherwise returns \c false.
*/
bool QSslCipher::isNull() const
{
    return d->isNull;
}

/*!
    Returns the name of the cipher, or an empty QString if this is a null
    cipher.

    \sa isNull()
*/
QString QSslCipher::name() const
{
    return d->name;
}

/*!
    Returns the number of bits supported by the cipher.

    \sa usedBits()
*/
int QSslCipher::supportedBits()const
{
    return d->supportedBits;
}

/*!
    Returns the number of bits used by the cipher.

    \sa supportedBits()
*/
int QSslCipher::usedBits() const
{
    return d->bits;
}

/*!
    Returns the cipher's key exchange method as a QString.
*/
QString QSslCipher::keyExchangeMethod() const
{
    return d->keyExchangeMethod;
}

/*!
    Returns the cipher's authentication method as a QString.
*/
QString QSslCipher::authenticationMethod() const
{
    return d->authenticationMethod;
}

/*!
    Returns the cipher's encryption method as a QString.
*/
QString QSslCipher::encryptionMethod() const
{
    return d->encryptionMethod;
}

/*!
    Returns the cipher's protocol as a QString.

    \sa protocol()
*/
QString QSslCipher::protocolString() const
{
    return d->protocolString;
}

/*!
    Returns the cipher's protocol type, or \l QSsl::UnknownProtocol if
    QSslCipher is unable to determine the protocol (protocolString() may
    contain more information).

    \sa protocolString()
*/
QSsl::SslProtocol QSslCipher::protocol() const
{
    return d->protocol;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QSslCipher &cipher)
{
    debug << "QSslCipher(name=" << qPrintable(cipher.name())
          << ", bits=" << cipher.usedBits()
          << ", proto=" << qPrintable(cipher.protocolString())
          << ')';
    return debug;
}
#endif

QT_END_NAMESPACE
