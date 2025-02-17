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


#include "qsslkey.h"
#include "qssl_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcSsl, "qt.network.ssl");

/*! \namespace QSsl

    \brief The QSsl namespace declares enums common to all SSL classes in Qt Network.
    \since 4.3

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork
*/

/*!
    \enum QSsl::KeyType

    Describes the two types of keys QSslKey supports.

    \value PrivateKey A private key.
    \value PublicKey A public key.
*/

/*!
    \enum QSsl::KeyAlgorithm

    Describes the different key algorithms supported by QSslKey.

    \value Rsa The RSA algorithm.
    \value Dsa The DSA algorithm.
    \value Opaque A key that should be treated as a 'black box' by QSslKey.

    The opaque key facility allows applications to add support for facilities
    such as PKCS#11 that Qt does not currently offer natively.
*/

/*!
    \enum QSsl::EncodingFormat

    Describes supported encoding formats for certificates and keys.

    \value Pem The PEM format.
    \value Der The DER format.
*/

/*!
    \enum QSsl::AlternativeNameEntryType

    Describes the key types for alternative name entries in QSslCertificate.

    \value EmailEntry An email entry; the entry contains an email address that
    the certificate is valid for.

    \value DnsEntry A DNS host name entry; the entry contains a host name
    entry that the certificate is valid for. The entry may contain wildcards.

    \note In Qt 4, this enum was called \c {AlternateNameEntryType}. That name
    is deprecated in Qt 5.

    \sa QSslCertificate::subjectAlternativeNames()
*/

/*!
  \typedef QSsl::AlternateNameEntryType
  \obsolete

  Use QSsl::AlternativeNameEntryType instead.
*/

/*!
    \enum QSsl::SslProtocol

    Describes the protocol of the cipher.

    \value SslV3 SSLv3
    \value SslV2 SSLv2
    \value TlsV1_0 TLSv1.0
    \value TlsV1 Obsolete, means the same as TlsV1_0
    \value TlsV1_1 TLSv1.1
    \value TlsV1_2 TLSv1.2
    \value UnknownProtocol The cipher's protocol cannot be determined.
    \value AnyProtocol The socket understands SSLv2, SSLv3, and TLSv1.0. This
    value is used by QSslSocket only.
    \value TlsV1SslV3 On the client side, this will send
    a TLS 1.0 Client Hello, enabling TLSv1_0 and SSLv3 connections.
    On the server side, this will enable both SSLv3 and TLSv1_0 connections.
    \value SecureProtocols The default option, using protocols known to be secure;
    currently behaves similar to TlsV1Ssl3 except denying SSLv3 connections that does
    not upgrade to TLS.

    \note most servers understand both SSL and TLS, but it is recommended to use
    TLS only for security reasons. However, SSL and TLS are not compatible with
    each other: if you get unexpected handshake failures, verify that you chose
    the correct setting for your protocol.
*/

/*!
    \enum QSsl::SslOption

    Describes the options that can be used to control the details of
    SSL behaviour. These options are generally used to turn features off
    to work around buggy servers.

    \value SslOptionDisableEmptyFragments Disables the insertion of empty
    fragments into the data when using block ciphers. When enabled, this
    prevents some attacks (such as the BEAST attack), however it is
    incompatible with some servers.
    \value SslOptionDisableSessionTickets Disables the SSL session ticket
    extension. This can cause slower connection setup, however some servers
    are not compatible with the extension.
    \value SslOptionDisableCompression Disables the SSL compression
    extension. When enabled, this allows the data being passed over SSL to
    be compressed, however some servers are not compatible with this
    extension.
    \value SslOptionDisableServerNameIndication Disables the SSL server
    name indication extension. When enabled, this tells the server the virtual
    host being accessed allowing it to respond with the correct certificate.
    \value SslOptionDisableLegacyRenegotiation Disables the older insecure
    mechanism for renegotiating the connection parameters. When enabled, this
    option can allow connections for legacy servers, but it introduces the
    possibility that an attacker could inject plaintext into the SSL session.
    \value SslOptionDisableSessionSharing Disables SSL session sharing via
    the session ID handshake attribute.
    \value SslOptionDisableSessionPersistence Disables storing the SSL session
    in ASN.1 format as returned by QSslConfiguration::sessionTicket(). Enabling
    this feature adds memory overhead of approximately 1K per used session
    ticket.

    By default, SslOptionDisableEmptyFragments is turned on since this causes
    problems with a large number of servers. SslOptionDisableLegacyRenegotiation
    is also turned on, since it introduces a security risk.
    SslOptionDisableCompression is turned on to prevent the attack publicised by
    CRIME.
    SslOptionDisableSessionPersistence is turned on to optimize memory usage.
    The other options are turned off.

    \note Availability of above options depends on the version of the SSL
    backend in use.
*/


QT_END_NAMESPACE
