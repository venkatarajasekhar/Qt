/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef _WINDOWDUMP_
#define _WINDOWDUMP_

#include <QtCore/qnamespace.h>

QT_FORWARD_DECLARE_CLASS(QRect)
QT_FORWARD_DECLARE_CLASS(QObject)
QT_FORWARD_DECLARE_CLASS(QWindow)
QT_FORWARD_DECLARE_CLASS(QTextStream)

namespace QtDiag {

enum FormatWindowOption {
    DontPrintWindowFlags = 0x001
};

Q_DECLARE_FLAGS(FormatWindowOptions, FormatWindowOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(FormatWindowOptions)

void indentStream(QTextStream &s, int indent);
void formatObject(QTextStream &str, const QObject *o);
void formatRect(QTextStream &str, const QRect &geom);
void formatWindowFlags(QTextStream &str, const Qt::WindowFlags flags);

void formatWindow(QTextStream &str, const QWindow *w, FormatWindowOptions options = 0);
void dumpAllWindows(FormatWindowOptions options = 0);

} // namespace QtDiag

#endif
