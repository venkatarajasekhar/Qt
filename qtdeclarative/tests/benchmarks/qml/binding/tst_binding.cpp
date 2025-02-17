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

#include <qtest.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QFile>
#include <QDebug>
#include "testtypes.h"

class tst_binding : public QObject
{
    Q_OBJECT

public:
    tst_binding();
    virtual ~tst_binding();

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void objectproperty_data();
    void objectproperty();
    void basicproperty_data();
    void basicproperty();
    void creation_data();
    void creation();

private:
    QQmlEngine engine;
    MyQmlObject tstObject;
};

tst_binding::tst_binding()
{
}

tst_binding::~tst_binding()
{
}

void tst_binding::initTestCase()
{
    registerTypes();
    engine.rootContext()->setContextProperty("tstObject", &tstObject);
}

void tst_binding::cleanupTestCase()
{
}

#define COMPONENT(filename, binding) \
    QQmlComponent c(&engine); \
    { \
        QFile f(filename); \
        QVERIFY(f.open(QIODevice::ReadOnly)); \
        QByteArray data = f.readAll(); \
        data.replace("###", binding.toUtf8()); \
        c.setData(data, QUrl()); \
        QVERIFY(c.isReady()); \
    }

void tst_binding::objectproperty_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("binding");

    QTest::newRow("object.value") << SRCDIR "/data/objectproperty.txt" << "object.value";
    QTest::newRow("object.value + 10") << SRCDIR "/data/objectproperty.txt" << "object.value + 10";
}

void tst_binding::objectproperty()
{
    QFETCH(QString, file);
    QFETCH(QString, binding);

    COMPONENT(file, binding);

    MyQmlObject object1;
    MyQmlObject object2;

    MyQmlObject *object = qobject_cast<MyQmlObject *>(c.create());
    QVERIFY(object != 0);
    object->setObject(&object2);

    QBENCHMARK {
        object->setObject(&object1);
        object->setObject(&object2);
    }
}

void tst_binding::basicproperty_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("binding");

    QTest::newRow("value") << SRCDIR "/data/localproperty.txt" << "value";
    QTest::newRow("value + 10") << SRCDIR "/data/localproperty.txt" << "value + 10";
    QTest::newRow("value + value + 10") << SRCDIR "/data/localproperty.txt" << "value + value + 10";

    QTest::newRow("myObject.value") << SRCDIR "/data/idproperty.txt" << "myObject.value";
    QTest::newRow("myObject.value + 10") << SRCDIR "/data/idproperty.txt" << "myObject.value + 10";
    QTest::newRow("myObject.value + myObject.value + 10") << SRCDIR "/data/idproperty.txt" << "myObject.value + myObject.value + 10";
}

void tst_binding::basicproperty()
{
    QFETCH(QString, file);
    QFETCH(QString, binding);

    COMPONENT(file, binding);

    MyQmlObject *object = qobject_cast<MyQmlObject *>(c.create());
    QVERIFY(object != 0);
    object->setValue(10);

    QBENCHMARK {
        object->setValue(1);
    }
}

void tst_binding::creation_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("binding");

    QTest::newRow("constant") << SRCDIR "/data/creation.txt" << "10";
    QTest::newRow("ownProperty") << SRCDIR "/data/creation.txt" << "myObject.value";
    QTest::newRow("declaredProperty") << SRCDIR "/data/creation.txt" << "myObject.myValue";
    QTest::newRow("contextProperty") << SRCDIR "/data/creation.txt" << "tstObject.value";
}

void tst_binding::creation()
{
    QFETCH(QString, file);
    QFETCH(QString, binding);

    COMPONENT(file, binding);

    QBENCHMARK {
        QObject *o = c.create();
        delete o;
    }
}

QTEST_MAIN(tst_binding)
#include "tst_binding.moc"
