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

#include "gridwidget.h"
#include <QGridLayout>
#include <QPushButton>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>

GridWidget::GridWidget(QWidget *parent) :
    QWidget(parent)
{
    QGridLayout *hb = new QGridLayout(this);
    hb->setObjectName("GridWidget");
    QComboBox *combo = new QComboBox();
    combo->addItem("123");
    QComboBox *combo2 = new QComboBox();
    combo2->setEditable(true);
    combo2->addItem("123");

    hb->addWidget(new QLabel("123"), 0, 0);
    hb->addWidget(new QLabel("123"), 0, 1);
    hb->addWidget(new QLineEdit("123"), 1, 0);
    hb->addWidget(new QLineEdit("123"), 1, 1);
    hb->addWidget(new QCheckBox("123"), 0, 2);
    hb->addWidget(new QCheckBox("123"), 1, 2);
    hb->addWidget(combo, 0, 3);
    hb->addWidget(combo2, 1, 3);
    hb->addWidget(new QDateTimeEdit(), 0, 4);
    hb->addWidget(new QPushButton("123"), 1, 4);
    hb->addWidget(new QSpinBox(), 0, 5);
    hb->addWidget(new QSpinBox(), 1, 5);

    qDebug("There should be four warnings, but no crash or freeze:");
    hb->addWidget(this, 6, 6); ///< This command should print a warning, but should not add "this"
    hb->addWidget(Q_NULLPTR, 6, 7); ///< This command should print a warning, but should not add "NULL"
    hb->addLayout(hb, 7, 6); ///< This command should print a warning, but should not add "hb"
    hb->addLayout(Q_NULLPTR, 7, 7); ///< This command should print a warning, but should not add "NULL"
    qDebug("Neither crashed nor frozen");
}
