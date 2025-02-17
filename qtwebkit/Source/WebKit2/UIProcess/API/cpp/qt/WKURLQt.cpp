/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "WKURLQt.h"

#include "WKAPICast.h"
#include "WKRetainPtr.h"
#include <QString>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

using namespace WebKit;

WKURLRef WKURLCreateWithQString(const QString& url)
{
    return toCopiedURLAPI(url);
}

QString WKURLCopyQString(WKURLRef urlRef)
{
    if (!urlRef)
        return QString();
    return toImpl(urlRef)->string();
}

WKURLRef WKURLCreateWithQUrl(const QUrl& url)
{
    return WKURLCreateWithQString(url.toString(QUrl::FullyEncoded));
}

QUrl WKURLCopyQUrl(WKURLRef urlRef)
{
    if (!urlRef)
        return QUrl();
    return QUrl(WKURLCopyQString(urlRef));
}

namespace WebKit {
QString adoptToQString(WKURLRef urlRef)
{
    return WKURLCopyQString(adoptWK(urlRef).get());
}

QUrl adoptToQUrl(WKURLRef urlRef)
{
    return WKURLCopyQUrl(adoptWK(urlRef).get());
}
} /* namespace WebKit */
