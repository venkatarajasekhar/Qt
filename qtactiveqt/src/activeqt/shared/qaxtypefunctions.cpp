/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the ActiveQt framework of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qt_windows.h>
#include "qaxtypefunctions.h"
#include <qcursor.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qrect.h>
#include <qpoint.h>

QT_BEGIN_NAMESPACE

QColor OLEColorToQColor(uint col)
{
    return QColor(GetRValue(col),GetGValue(col),GetBValue(col));
}

/*!
    Copies the data in \a var into \a data.

    Used by

    QAxServerBase:
        - QAxServerBase::qt_metacall (update out parameters/return value)

    QAxBase:
        - internalProperty(ReadProperty)
        - internalInvoke(update out parameters/return value)

*/
bool QVariantToVoidStar(const QVariant &var, void *data, const QByteArray &typeName, uint type)
{
    if (!data)
        return true;

    if (type == QMetaType::QVariant || type == QVariant::LastType || (type == 0 && typeName == "QVariant")) {
        *(QVariant*)data = var;
        return true;
    }

    switch (var.type()) {
    case QVariant::Invalid:
        break;
    case QVariant::String:
        *(QString*)data = var.toString();
        break;
    case QVariant::Int:
        *(int*)data = var.toInt();
        break;
    case QVariant::UInt:
        *(uint*)data = var.toUInt();
        break;
    case QVariant::Bool:
        *(bool*)data = var.toBool();
        break;
    case QVariant::Double:
        *(double*)data = var.toDouble();
        break;
    case QVariant::Color:
        *(QColor*)data = qvariant_cast<QColor>(var);
        break;
    case QVariant::Date:
        *(QDate*)data = var.toDate();
        break;
    case QVariant::Time:
        *(QTime*)data = var.toTime();
        break;
    case QVariant::DateTime:
        *(QDateTime*)data = var.toDateTime();
        break;
    case QVariant::Font:
        *(QFont*)data = qvariant_cast<QFont>(var);
        break;
    case QVariant::Pixmap:
        *(QPixmap*)data = qvariant_cast<QPixmap>(var);
        break;
#ifndef QT_NO_CURSOR
    case QVariant::Cursor:
        *(QCursor*)data = qvariant_cast<QCursor>(var);
        break;
#endif
    case QVariant::List:
        *(QList<QVariant>*)data = var.toList();
        break;
    case QVariant::StringList:
        *(QStringList*)data = var.toStringList();
        break;
    case QVariant::ByteArray:
        *(QByteArray*)data = var.toByteArray();
        break;
    case QVariant::LongLong:
        *(qint64*)data = var.toLongLong();
        break;
    case QVariant::ULongLong:
        *(quint64*)data = var.toULongLong();
        break;
    case QVariant::Rect:
        *(QRect*)data = var.toRect();
        break;
    case QVariant::Size:
        *(QSize*)data = var.toSize();
        break;
    case QVariant::Point:
        *(QPoint*)data = var.toPoint();
        break;
    case QVariant::UserType:
        *(void**)data = *(void**)var.constData();
//        qVariantGet(var, *(void**)data, typeName);
        break;
    default:
        qWarning("QVariantToVoidStar: Unhandled QVariant type");
        return false;
    }

    return true;
}

void clearVARIANT(VARIANT *var)
{
    if (var->vt & VT_BYREF) {
        switch (var->vt) {
        case VT_BSTR|VT_BYREF:
            SysFreeString(*var->pbstrVal);
            delete var->pbstrVal;
            break;
        case VT_BOOL|VT_BYREF:
            delete var->pboolVal;
            break;
        case VT_I1|VT_BYREF:
            delete var->pcVal;
            break;
        case VT_I2|VT_BYREF:
            delete var->piVal;
            break;
        case VT_I4|VT_BYREF:
            delete var->plVal;
            break;
        case VT_INT|VT_BYREF:
            delete var->pintVal;
            break;
        case VT_UI1|VT_BYREF:
            delete var->pbVal;
            break;
        case VT_UI2|VT_BYREF:
            delete var->puiVal;
            break;
        case VT_UI4|VT_BYREF:
            delete var->pulVal;
            break;
        case VT_UINT|VT_BYREF:
            delete var->puintVal;
            break;
#if !defined(Q_OS_WINCE) && defined(_MSC_VER) && _MSC_VER >= 1400
        case VT_I8|VT_BYREF:
            delete var->pllVal;
            break;
        case VT_UI8|VT_BYREF:
            delete var->pullVal;
            break;
#endif
        case VT_CY|VT_BYREF:
            delete var->pcyVal;
            break;
        case VT_R4|VT_BYREF:
            delete var->pfltVal;
            break;
        case VT_R8|VT_BYREF:
            delete var->pdblVal;
            break;
        case VT_DATE|VT_BYREF:
            delete var->pdate;
            break;
        case VT_DISPATCH|VT_BYREF:
            (*var->ppdispVal)->Release();
            delete var->ppdispVal;
            break;
        case VT_ARRAY|VT_VARIANT|VT_BYREF:
        case VT_ARRAY|VT_UI1|VT_BYREF:
        case VT_ARRAY|VT_BSTR|VT_BYREF:
            SafeArrayDestroy(*var->pparray);
            delete var->pparray;
            break;
        case VT_VARIANT|VT_BYREF:
            delete var->pvarVal;
            break;
        }
        VariantInit(var);
    } else {
        VariantClear(var);
    }
}

QT_END_NAMESPACE
