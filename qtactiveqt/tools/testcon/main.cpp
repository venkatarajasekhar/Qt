/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "mainwindow.h"

#include <QApplication>
#include <QAxFactory>

QAXFACTORY_DEFAULT(MainWindow,
                   QLatin1String("{5f5ce700-48a8-47b1-9b06-3b7f79e41d7c}"),
                   QLatin1String("{3fc86f5f-8b15-4428-8f6b-482bae91f1ae}"),
                   QLatin1String("{02a268cd-24b4-4fd9-88ff-b01b683ef39d}"),
                   QLatin1String("{4a43e44d-9d1d-47e5-a1e5-58fe6f7be0a4}"),
                   QLatin1String("{16ee5998-77d2-412f-ad91-8596e29f123f}"))

QT_USE_NAMESPACE

int main( int argc, char **argv )
{
    QApplication app( argc, argv );

    MainWindow mw;
    mw.show();

    return app.exec();;
}
