/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <private/qserialportinfo_p.h>

class tst_QSerialPortInfoPrivate : public QObject
{
    Q_OBJECT
public:
    explicit tst_QSerialPortInfoPrivate();

private slots:
    void canonical_data();
    void canonical();
};

tst_QSerialPortInfoPrivate::tst_QSerialPortInfoPrivate()
{
}

void tst_QSerialPortInfoPrivate::canonical_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("location");

#if defined (Q_OS_WINCE)
    QTest::newRow("Test1") << "COM1" << "COM1" << "COM1:";
    QTest::newRow("Test2") << "COM1:" << "COM1" << "COM1:";
#elif defined (Q_OS_WIN32)
    QTest::newRow("Test1") << "COM1" << "COM1" << "\\\\.\\COM1";
    QTest::newRow("Test2") << "\\\\.\\COM1" << "COM1" << "\\\\.\\COM1";
    QTest::newRow("Test3") << "//./COM1" << "COM1" << "//./COM1";
#elif defined (Q_OS_OSX)
    QTest::newRow("Test1") << "ttyS0" << "ttyS0" << "/dev/ttyS0";
    QTest::newRow("Test2") << "cu.serial1" << "cu.serial1" << "/dev/cu.serial1";
    QTest::newRow("Test3") << "tty.serial1" << "tty.serial1" << "/dev/tty.serial1";
    QTest::newRow("Test4") << "/dev/ttyS0" << "ttyS0" << "/dev/ttyS0";
    QTest::newRow("Test5") << "/dev/tty.serial1" << "tty.serial1" << "/dev/tty.serial1";
    QTest::newRow("Test6") << "/dev/cu.serial1" << "cu.serial1" << "/dev/cu.serial1";
    QTest::newRow("Test7") << "/dev/serial/ttyS0" << "serial/ttyS0" << "/dev/serial/ttyS0";
    QTest::newRow("Test8") << "/home/ttyS0" << "/home/ttyS0" << "/home/ttyS0";
    QTest::newRow("Test9") << "/home/serial/ttyS0" << "/home/serial/ttyS0" << "/home/serial/ttyS0";
    QTest::newRow("Test10") << "serial/ttyS0" << "serial/ttyS0" << "/dev/serial/ttyS0";
    QTest::newRow("Test11") << "./ttyS0" << "./ttyS0" << "./ttyS0";
    QTest::newRow("Test12") << "../ttyS0" << "../ttyS0" << "../ttyS0";
#elif defined (Q_OS_UNIX)
    QTest::newRow("Test1") << "ttyS0" << "ttyS0" << "/dev/ttyS0";
    QTest::newRow("Test2") << "/dev/ttyS0" << "ttyS0" << "/dev/ttyS0";
    QTest::newRow("Test3") << "/dev/serial/ttyS0" << "serial/ttyS0" << "/dev/serial/ttyS0";
    QTest::newRow("Test4") << "/home/ttyS0" << "/home/ttyS0" << "/home/ttyS0";
    QTest::newRow("Test5") << "/home/serial/ttyS0" << "/home/serial/ttyS0" << "/home/serial/ttyS0";
    QTest::newRow("Test6") << "serial/ttyS0" << "serial/ttyS0" << "/dev/serial/ttyS0";
    QTest::newRow("Test7") << "./ttyS0" << "./ttyS0" << "./ttyS0";
    QTest::newRow("Test8") << "../ttyS0" << "../ttyS0" << "../ttyS0";
#endif
}

void tst_QSerialPortInfoPrivate::canonical()
{
    QFETCH(QString, source);
    QFETCH(QString, name);
    QFETCH(QString, location);

    QCOMPARE(QSerialPortInfoPrivate::portNameFromSystemLocation(source), name);
    QCOMPARE(QSerialPortInfoPrivate::portNameToSystemLocation(source), location);
}

QTEST_MAIN(tst_QSerialPortInfoPrivate)
#include "tst_qserialportinfoprivate.moc"
