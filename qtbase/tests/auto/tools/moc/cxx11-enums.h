/****************************************************************************
**
** Copyright (C) 2011 Olivier Goffart.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef CXX11_ENUMS_H
#define CXX11_ENUMS_H
#include <QtCore/QObject>

#if defined(Q_COMPILER_CLASS_ENUM) || defined(Q_MOC_RUN)
class CXX11Enums
{
    Q_GADGET
public:
    Q_ENUMS(EnumClass TypedEnum TypedEnumClass NormalEnum)
    enum class EnumClass { A0, A1, A2, A3 };
    enum TypedEnum : char { B0, B1 , B2, B3 };
    enum class TypedEnumClass : char { C0, C1, C2, C3 };
    enum NormalEnum { D2 = 2, D3, D0 =0 , D1 };
};

#else
//workaround to get the moc compiled code to compile
class CXX11Enums
{
    Q_GADGET
public:
    struct EnumClass { enum { A0, A1, A2, A3 }; };
    struct TypedEnumClass { enum { C0, C1, C2, C3 }; };
    enum NormalEnum { D2 = 2, D3, D0 =0 , D1 };
    enum TypedEnum { B0, B1 , B2, B3 };
};
#endif
#endif // CXX11_ENUMS_H
