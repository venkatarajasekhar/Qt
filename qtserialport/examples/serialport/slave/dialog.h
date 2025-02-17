/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
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

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QTimer>

#include <QtSerialPort/QSerialPort>

QT_USE_NAMESPACE

QT_BEGIN_NAMESPACE

class QLabel;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QPushButton;

QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = 0);

private slots:
    void startSlave();
    void readRequest();
    void processTimeout();
    void activateRunButton();

private:
    void processError(const QString &s);

private:
    int transactionCount;
    QLabel *serialPortLabel;
    QComboBox *serialPortComboBox;
    QLabel *waitRequestLabel;
    QSpinBox *waitRequestSpinBox;
    QLabel *responseLabel;
    QLineEdit *responseLineEdit;
    QLabel *trafficLabel;
    QLabel *statusLabel;
    QPushButton *runButton;

    QSerialPort serial;
    QByteArray request;
    QTimer timer;
};

#endif // DIALOG_H
