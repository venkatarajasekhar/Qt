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
#include <QQmlComponent>
#include <QDebug>
#include <QScopedPointer>
#include <QNetworkCookieJar>
#include "testhttpserver.h"
#include "../../shared/util.h"

#define SERVER_PORT 14445

class tst_qqmlxmlhttprequest : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlxmlhttprequest() {}

private slots:
    void domExceptionCodes();
    void callbackException();
    void callbackException_data();
    void staticStateValues();
    void instanceStateValues();
    void constructor();
    void defaultState();
    void open();
    void open_data();
    void open_invalid_method();
    void open_sync();
    void open_arg_count();
    void setRequestHeader();
    void setRequestHeader_caseInsensitive();
    void setRequestHeader_unsent();
    void setRequestHeader_illegalName_data();
    void setRequestHeader_illegalName();
    void setRequestHeader_sent();
    void setRequestHeader_args();
    void send_unsent();
    void send_alreadySent();
    void send_ignoreData();
    void send_withdata();
    void send_withdata_data();
    void abort();
    void abort_unsent();
    void abort_opened();
    void getResponseHeader();
    void getResponseHeader_unsent();
    void getResponseHeader_sent();
    void getResponseHeader_args();
    void getAllResponseHeaders();
    void getAllResponseHeaders_unsent();
    void getAllResponseHeaders_sent();
    void getAllResponseHeaders_args();
    void status();
    void status_data();
    void statusText();
    void statusText_data();
    void responseText();
    void responseText_data();
    void responseXML_invalid();
    void invalidMethodUsage();
    void redirects();
    void nonUtf8();
    void nonUtf8_data();

    // Attributes
    void document();
    void element();
    void attr();
    void text();
    void cdata();

    // Crashes
    // void outstanding_request_at_shutdown();

    // void network_errors()
    // void readyState()

    void stateChangeCallingContext();

private:
    QQmlEngine engine;
};

// Test that the dom exception codes are correct
void tst_qqmlxmlhttprequest::domExceptionCodes()
{
    QQmlComponent component(&engine, testFileUrl("domExceptionCodes.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("index_size_err").toInt(), 1);
    QCOMPARE(object->property("domstring_size_err").toInt(), 2);
    QCOMPARE(object->property("hierarchy_request_err").toInt(), 3);
    QCOMPARE(object->property("wrong_document_err").toInt(), 4);
    QCOMPARE(object->property("invalid_character_err").toInt(), 5);
    QCOMPARE(object->property("no_data_allowed_err").toInt(), 6);
    QCOMPARE(object->property("no_modification_allowed_err").toInt(), 7);
    QCOMPARE(object->property("not_found_err").toInt(), 8);
    QCOMPARE(object->property("not_supported_err").toInt(), 9);
    QCOMPARE(object->property("inuse_attribute_err").toInt(), 10);
    QCOMPARE(object->property("invalid_state_err").toInt(), 11);
    QCOMPARE(object->property("syntax_err").toInt(), 12);
    QCOMPARE(object->property("invalid_modification_err").toInt(), 13);
    QCOMPARE(object->property("namespace_err").toInt(), 14);
    QCOMPARE(object->property("invalid_access_err").toInt(), 15);
    QCOMPARE(object->property("validation_err").toInt(), 16);
    QCOMPARE(object->property("type_mismatch_err").toInt(), 17);
}

void tst_qqmlxmlhttprequest::callbackException_data()
{
    QTest::addColumn<QString>("which");
    QTest::addColumn<int>("line");

    QTest::newRow("on-opened") << "1" << 15;
    QTest::newRow("on-loading") << "3" << 15;
    QTest::newRow("on-done") << "4" << 15;
}

void tst_qqmlxmlhttprequest::callbackException()
{
    // Test exception reporting for exceptions thrown at various points.

    QFETCH(QString, which);
    QFETCH(int, line);

    QString expect = testFileUrl("callbackException.qml").toString() + ":"+QString::number(line)+": Error: Exception from Callback";
    QTest::ignoreMessage(QtWarningMsg, expect.toLatin1());

    QQmlComponent component(&engine, testFileUrl("callbackException.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", "testdocument.html");
    object->setProperty("which", which);
    component.completeCreate();

    QTRY_VERIFY(object->property("threw").toBool() == true);
}

// Test that the state value properties on the XMLHttpRequest constructor have the correct values.
// ### WebKit does not do this, but it seems to fit the standard and QML better
void tst_qqmlxmlhttprequest::staticStateValues()
{
    QQmlComponent component(&engine, testFileUrl("staticStateValues.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("unsent").toInt(), 0);
    QCOMPARE(object->property("opened").toInt(), 1);
    QCOMPARE(object->property("headers_received").toInt(), 2);
    QCOMPARE(object->property("loading").toInt(), 3);
    QCOMPARE(object->property("done").toInt(), 4);
}

// Test that the state value properties on instances have the correct values.
void tst_qqmlxmlhttprequest::instanceStateValues()
{
    QQmlComponent component(&engine, testFileUrl("instanceStateValues.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("unsent").toInt(), 0);
    QCOMPARE(object->property("opened").toInt(), 1);
    QCOMPARE(object->property("headers_received").toInt(), 2);
    QCOMPARE(object->property("loading").toInt(), 3);
    QCOMPARE(object->property("done").toInt(), 4);
}

// Test calling constructor
void tst_qqmlxmlhttprequest::constructor()
{
    QQmlComponent component(&engine, testFileUrl("constructor.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("calledAsConstructor").toBool(), true);
    QCOMPARE(object->property("calledAsFunction").toBool(), true);
}

// Test that all the properties are set correctly before any request is sent
void tst_qqmlxmlhttprequest::defaultState()
{
    QQmlComponent component(&engine, testFileUrl("defaultState.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("readState").toInt(), 0);
    QCOMPARE(object->property("statusIsException").toBool(), true);
    QCOMPARE(object->property("statusTextIsException").toBool(), true);
    QCOMPARE(object->property("responseText").toString(), QString());
    QCOMPARE(object->property("responseXMLIsNull").toBool(), true);
}

// Test valid XMLHttpRequest.open() calls
void tst_qqmlxmlhttprequest::open()
{
    QFETCH(QUrl, qmlFile);
    QFETCH(QString, url);
    QFETCH(bool, remote);

    TestHTTPServer server;
    if (remote) {
        QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
        QVERIFY(server.wait(testFileUrl("open_network.expect"),
                            testFileUrl("open_network.reply"),
                            testFileUrl("testdocument.html")));
    }

    QQmlComponent component(&engine, qmlFile);
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", url);
    component.completeCreate();

    QCOMPARE(object->property("readyState").toBool(), true);
    QCOMPARE(object->property("openedState").toBool(), true);
    QCOMPARE(object->property("status").toBool(), true);
    QCOMPARE(object->property("statusText").toBool(), true);
    QCOMPARE(object->property("responseText").toBool(), true);
    QCOMPARE(object->property("responseXML").toBool(), true);

    QTRY_VERIFY(object->property("dataOK").toBool() == true);
}

void tst_qqmlxmlhttprequest::open_data()
{
    QTest::addColumn<QUrl>("qmlFile");
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("remote");

    QTest::newRow("Relative url)") << testFileUrl("open.qml") << "testdocument.html" << false;
    QTest::newRow("Absolute url)") << testFileUrl("open.qml") << testFileUrl("testdocument.html").toString() << false;
    QTest::newRow("Absolute network url)") << testFileUrl("open.qml") << "http://127.0.0.1:14445/testdocument.html" << true;

    // ### Check that the username/password were sent to the server
    QTest::newRow("User/pass") << testFileUrl("open_user.qml") << "http://127.0.0.1:14445/testdocument.html" << true;
}

// Test that calling XMLHttpRequest.open() with an invalid method raises an exception
void tst_qqmlxmlhttprequest::open_invalid_method()
{
    QQmlComponent component(&engine, testFileUrl("open_invalid_method.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("exceptionThrown").toBool(), true);
}

// Test that calling XMLHttpRequest.open() with sync raises an exception
void tst_qqmlxmlhttprequest::open_sync()
{
    QQmlComponent component(&engine, testFileUrl("open_sync.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("exceptionThrown").toBool(), true);
}

// Calling with incorrect arg count raises an exception
void tst_qqmlxmlhttprequest::open_arg_count()
{
    {
        QQmlComponent component(&engine, testFileUrl("open_arg_count.1.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());

        QCOMPARE(object->property("exceptionThrown").toBool(), true);
    }

    {
        QQmlComponent component(&engine, testFileUrl("open_arg_count.2.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());

        QCOMPARE(object->property("exceptionThrown").toBool(), true);
    }
}

// Test valid setRequestHeader() calls
void tst_qqmlxmlhttprequest::setRequestHeader()
{
    TestHTTPServer server;
    QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
    QVERIFY(server.wait(testFileUrl("setRequestHeader.expect"),
                        testFileUrl("setRequestHeader.reply"),
                        testFileUrl("testdocument.html")));

    QQmlComponent component(&engine, testFileUrl("setRequestHeader.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
    component.completeCreate();

    QTRY_VERIFY(object->property("dataOK").toBool() == true);
}

// Test valid setRequestHeader() calls with different header cases
void tst_qqmlxmlhttprequest::setRequestHeader_caseInsensitive()
{
    TestHTTPServer server;
    QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
    QVERIFY(server.wait(testFileUrl("setRequestHeader.expect"),
                        testFileUrl("setRequestHeader.reply"),
                        testFileUrl("testdocument.html")));

    QQmlComponent component(&engine, testFileUrl("setRequestHeader_caseInsensitive.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
    component.completeCreate();

    QTRY_VERIFY(object->property("dataOK").toBool() == true);
}
// Test setting headers before open() throws exception
void tst_qqmlxmlhttprequest::setRequestHeader_unsent()
{
    QQmlComponent component(&engine, testFileUrl("setRequestHeader_unsent.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("test").toBool(), true);
}

void tst_qqmlxmlhttprequest::setRequestHeader_illegalName_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("Accept-Charset") << "AccePT-CHArset";
    QTest::newRow("Accept-Encoding") << "AccEpt-EnCOding";
    QTest::newRow("Connection") << "ConnECtion";
    QTest::newRow("Content-Length") << "ContEnt-LenGth";
    QTest::newRow("Cookie") << "CookIe";
    QTest::newRow("Cookie2") << "CoOkie2";
    QTest::newRow("Content-Transfer-Encoding") << "ConteNT-tRANSFER-eNCOding";
    QTest::newRow("Date") << "DaTE";
    QTest::newRow("Expect") << "ExPect";
    QTest::newRow("Host") << "HoST";
    QTest::newRow("Keep-Alive") << "KEEP-aLive";
    QTest::newRow("Referer") << "ReferEr";
    QTest::newRow("TE") << "Te";
    QTest::newRow("Trailer") << "TraILEr";
    QTest::newRow("Transfer-Encoding") << "tRANsfer-Encoding";
    QTest::newRow("Upgrade") << "UpgrADe";
    QTest::newRow("User-Agent") << "uSEr-Agent";
    QTest::newRow("Via") << "vIa";
    QTest::newRow("Proxy-") << "ProXy-";
    QTest::newRow("Sec-") << "SeC-";
    QTest::newRow("Proxy-*") << "Proxy-BLAH";
    QTest::newRow("Sec-*") << "Sec-F";
}

// Tests that using illegal header names has no effect
void tst_qqmlxmlhttprequest::setRequestHeader_illegalName()
{
    QFETCH(QString, name);

    TestHTTPServer server;
    QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
    QVERIFY(server.wait(testFileUrl("open_network.expect"),
                        testFileUrl("open_network.reply"),
                        testFileUrl("testdocument.html")));

    QQmlComponent component(&engine, testFileUrl("setRequestHeader_illegalName.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
    object->setProperty("header", name);
    component.completeCreate();

    QCOMPARE(object->property("readyState").toBool(), true);
    QCOMPARE(object->property("openedState").toBool(), true);
    QCOMPARE(object->property("status").toBool(), true);
    QCOMPARE(object->property("statusText").toBool(), true);
    QCOMPARE(object->property("responseText").toBool(), true);
    QCOMPARE(object->property("responseXML").toBool(), true);

    QTRY_VERIFY(object->property("dataOK").toBool() == true);
}

// Test that attempting to set a header after a request is sent throws an exception
void tst_qqmlxmlhttprequest::setRequestHeader_sent()
{
    TestHTTPServer server;
    QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
    QVERIFY(server.wait(testFileUrl("open_network.expect"),
                        testFileUrl("open_network.reply"),
                        testFileUrl("testdocument.html")));

    QQmlComponent component(&engine, testFileUrl("setRequestHeader_sent.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
    component.completeCreate();

    QCOMPARE(object->property("test").toBool(), true);

    QTRY_VERIFY(object->property("dataOK").toBool() == true);
}

// Invalid arg count throws exception
void tst_qqmlxmlhttprequest::setRequestHeader_args()
{
    QQmlComponent component(&engine, testFileUrl("setRequestHeader_args.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("exceptionThrown").toBool(), true);
}

// Test that calling send() in UNSENT state throws an exception
void tst_qqmlxmlhttprequest::send_unsent()
{
    QQmlComponent component(&engine, testFileUrl("send_unsent.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("test").toBool(), true);
}

// Test attempting to resend a sent request throws an exception
void tst_qqmlxmlhttprequest::send_alreadySent()
{
    QQmlComponent component(&engine, testFileUrl("send_alreadySent.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("test").toBool(), true);
    QTRY_VERIFY(object->property("dataOK").toBool() == true);
}

// Test that sends for GET, HEAD and DELETE ignore data
void tst_qqmlxmlhttprequest::send_ignoreData()
{
    {
        TestHTTPServer server;
        QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
        QVERIFY(server.wait(testFileUrl("send_ignoreData_GET.expect"),
                            testFileUrl("send_ignoreData.reply"),
                            testFileUrl("testdocument.html")));

        QQmlComponent component(&engine, testFileUrl("send_ignoreData.qml"));
        QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
        QVERIFY(!object.isNull());
        object->setProperty("reqType", "GET");
        object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
        component.completeCreate();

        QTRY_VERIFY(object->property("dataOK").toBool() == true);
    }

    {
        TestHTTPServer server;
        QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
        QVERIFY(server.wait(testFileUrl("send_ignoreData_HEAD.expect"),
                            testFileUrl("send_ignoreData.reply"),
                            QUrl()));

        QQmlComponent component(&engine, testFileUrl("send_ignoreData.qml"));
        QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
        QVERIFY(!object.isNull());
        object->setProperty("reqType", "HEAD");
        object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
        component.completeCreate();

        QTRY_VERIFY(object->property("dataOK").toBool() == true);
    }

    {
        TestHTTPServer server;
        QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
        QVERIFY(server.wait(testFileUrl("send_ignoreData_DELETE.expect"),
                            testFileUrl("send_ignoreData.reply"),
                            QUrl()));

        QQmlComponent component(&engine, testFileUrl("send_ignoreData.qml"));
        QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
        QVERIFY(!object.isNull());
        object->setProperty("reqType", "DELETE");
        object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
        component.completeCreate();

        QTRY_VERIFY(object->property("dataOK").toBool() == true);
    }
}

// Test that send()'ing data works
void tst_qqmlxmlhttprequest::send_withdata()
{
    QFETCH(QString, file_expected);
    QFETCH(QString, file_qml);

    TestHTTPServer server;
    QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
    QVERIFY(server.wait(testFileUrl(file_expected),
                        testFileUrl("send_data.reply"),
                        testFileUrl("testdocument.html")));

    QQmlComponent component(&engine, testFileUrl(file_qml));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
    component.completeCreate();

    QTRY_VERIFY(object->property("dataOK").toBool() == true);
}

void tst_qqmlxmlhttprequest::send_withdata_data()
{
    QTest::addColumn<QString>("file_expected");
    QTest::addColumn<QString>("file_qml");

    QTest::newRow("No content-type") << "send_data.1.expect" << "send_data.1.qml";
    QTest::newRow("Correct content-type") << "send_data.1.expect" << "send_data.2.qml";
    QTest::newRow("Incorrect content-type") << "send_data.1.expect" << "send_data.3.qml";
    QTest::newRow("Correct content-type - out of order") << "send_data.4.expect" << "send_data.4.qml";
    QTest::newRow("Incorrect content-type - out of order") << "send_data.4.expect" << "send_data.5.qml";
    QTest::newRow("PUT") << "send_data.6.expect" << "send_data.6.qml";
    QTest::newRow("Correct content-type - no charset") << "send_data.1.expect" << "send_data.7.qml";
}

// Test abort() has no effect in unsent state
void tst_qqmlxmlhttprequest::abort_unsent()
{
    QQmlComponent component(&engine, testFileUrl("abort_unsent.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", "testdocument.html");
    component.completeCreate();

    QCOMPARE(object->property("readyState").toBool(), true);
    QCOMPARE(object->property("openedState").toBool(), true);
    QCOMPARE(object->property("status").toBool(), true);
    QCOMPARE(object->property("statusText").toBool(), true);
    QCOMPARE(object->property("responseText").toBool(), true);
    QCOMPARE(object->property("responseXML").toBool(), true);

    QTRY_VERIFY(object->property("dataOK").toBool() == true);
}

// Test abort() cancels an open (but unsent) request
void tst_qqmlxmlhttprequest::abort_opened()
{
    QQmlComponent component(&engine, testFileUrl("abort_opened.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", "testdocument.html");
    component.completeCreate();

    QCOMPARE(object->property("readyState").toBool(), true);
    QCOMPARE(object->property("openedState").toBool(), true);
    QCOMPARE(object->property("status").toBool(), true);
    QCOMPARE(object->property("statusText").toBool(), true);
    QCOMPARE(object->property("responseText").toBool(), true);
    QCOMPARE(object->property("responseXML").toBool(), true);

    QTRY_VERIFY(object->property("dataOK").toBool() == true);
}

// Test abort() aborts in progress send
void tst_qqmlxmlhttprequest::abort()
{
    TestHTTPServer server;
    QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
    QVERIFY(server.wait(testFileUrl("abort.expect"),
                        testFileUrl("abort.reply"),
                        testFileUrl("testdocument.html")));

    QQmlComponent component(&engine, testFileUrl("abort.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("urlDummy", "http://127.0.0.1:14449/testdocument.html");
    object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
    component.completeCreate();

    QCOMPARE(object->property("seenDone").toBool(), true);
    QCOMPARE(object->property("didNotSeeUnsent").toBool(), true);
    QCOMPARE(object->property("endStateUnsent").toBool(), true);

    QTRY_VERIFY(object->property("dataOK").toBool() == true);
}

void tst_qqmlxmlhttprequest::getResponseHeader()
{
    QQmlEngine engine; // Avoid cookie contamination

    TestHTTPServer server;
    QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
    QVERIFY(server.wait(testFileUrl("getResponseHeader.expect"),
                        testFileUrl("getResponseHeader.reply"),
                        testFileUrl("testdocument.html")));


    QQmlComponent component(&engine, testFileUrl("getResponseHeader.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
    component.completeCreate();

    QCOMPARE(object->property("unsentException").toBool(), true);
    QCOMPARE(object->property("openedException").toBool(), true);
    QCOMPARE(object->property("readyState").toBool(), true);
    QCOMPARE(object->property("openedState").toBool(), true);

    QTRY_VERIFY(object->property("dataOK").toBool() == true);

    QCOMPARE(object->property("headersReceivedState").toBool(), true);
    QCOMPARE(object->property("headersReceivedNullHeader").toBool(), true);
    QCOMPARE(object->property("headersReceivedValidHeader").toBool(), true);
    QCOMPARE(object->property("headersReceivedMultiValidHeader").toBool(), true);
    QCOMPARE(object->property("headersReceivedCookieHeader").toBool(), true);

    QCOMPARE(object->property("doneState").toBool(), true);
    QCOMPARE(object->property("doneNullHeader").toBool(), true);
    QCOMPARE(object->property("doneValidHeader").toBool(), true);
    QCOMPARE(object->property("doneMultiValidHeader").toBool(), true);
    QCOMPARE(object->property("doneCookieHeader").toBool(), true);
}

// Test getResponseHeader throws an exception in an invalid state
void tst_qqmlxmlhttprequest::getResponseHeader_unsent()
{
    QQmlComponent component(&engine, testFileUrl("getResponseHeader_unsent.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("test").toBool(), true);
}

// Test getResponseHeader throws an exception in an invalid state
void tst_qqmlxmlhttprequest::getResponseHeader_sent()
{
    QQmlComponent component(&engine, testFileUrl("getResponseHeader_sent.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("test").toBool(), true);
}

// Invalid arg count throws exception
void tst_qqmlxmlhttprequest::getResponseHeader_args()
{
    QQmlComponent component(&engine, testFileUrl("getResponseHeader_args.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QTRY_VERIFY(object->property("exceptionThrown").toBool() == true);
}

void tst_qqmlxmlhttprequest::getAllResponseHeaders()
{
    QQmlEngine engine; // Avoid cookie contamination

    TestHTTPServer server;
    QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
    QVERIFY(server.wait(testFileUrl("getResponseHeader.expect"),
                        testFileUrl("getResponseHeader.reply"),
                        testFileUrl("testdocument.html")));

    QQmlComponent component(&engine, testFileUrl("getAllResponseHeaders.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
    component.completeCreate();

    QCOMPARE(object->property("unsentException").toBool(), true);
    QCOMPARE(object->property("openedException").toBool(), true);
    QCOMPARE(object->property("readyState").toBool(), true);
    QCOMPARE(object->property("openedState").toBool(), true);

    QTRY_VERIFY(object->property("dataOK").toBool() == true);

    QCOMPARE(object->property("headersReceivedState").toBool(), true);
    QCOMPARE(object->property("headersReceivedHeader").toBool(), true);

    QCOMPARE(object->property("doneState").toBool(), true);
    QCOMPARE(object->property("doneHeader").toBool(), true);
}

// Test getAllResponseHeaders throws an exception in an invalid state
void tst_qqmlxmlhttprequest::getAllResponseHeaders_unsent()
{
    QQmlComponent component(&engine, testFileUrl("getAllResponseHeaders_unsent.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("test").toBool(), true);
}

// Test getAllResponseHeaders throws an exception in an invalid state
void tst_qqmlxmlhttprequest::getAllResponseHeaders_sent()
{
    QQmlComponent component(&engine, testFileUrl("getAllResponseHeaders_sent.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("test").toBool(), true);
}

// Invalid arg count throws exception
void tst_qqmlxmlhttprequest::getAllResponseHeaders_args()
{
    QQmlComponent component(&engine, testFileUrl("getAllResponseHeaders_args.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QTRY_VERIFY(object->property("exceptionThrown").toBool() == true);
}

void tst_qqmlxmlhttprequest::status()
{
    QFETCH(QUrl, replyUrl);
    QFETCH(int, status);

    TestHTTPServer server;
    QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
    QVERIFY(server.wait(testFileUrl("status.expect"),
                        replyUrl,
                        testFileUrl("testdocument.html")));

    QQmlComponent component(&engine, testFileUrl("status.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
    object->setProperty("expectedStatus", status);
    component.completeCreate();

    QTRY_VERIFY(object->property("dataOK").toBool() == true);

    QCOMPARE(object->property("unsentException").toBool(), true);
    QCOMPARE(object->property("openedException").toBool(), true);
    QCOMPARE(object->property("sentException").toBool(), true);
    QCOMPARE(object->property("headersReceived").toBool(), true);
    QCOMPARE(object->property("loading").toBool(), true);
    QCOMPARE(object->property("done").toBool(), true);
    QCOMPARE(object->property("resetException").toBool(), true);
}

void tst_qqmlxmlhttprequest::status_data()
{
    QTest::addColumn<QUrl>("replyUrl");
    QTest::addColumn<int>("status");

    QTest::newRow("OK") << testFileUrl("status.200.reply") << 200;
    QTest::newRow("Not Found") << testFileUrl("status.404.reply") << 404;
    QTest::newRow("Bad Request") << testFileUrl("status.400.reply") << 400;
}

void tst_qqmlxmlhttprequest::statusText()
{
    QFETCH(QUrl, replyUrl);
    QFETCH(QString, statusText);

    TestHTTPServer server;
    QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
    QVERIFY(server.wait(testFileUrl("status.expect"),
                        replyUrl,
                        testFileUrl("testdocument.html")));

    QQmlComponent component(&engine, testFileUrl("statusText.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
    object->setProperty("expectedStatus", statusText);
    component.completeCreate();

    QTRY_VERIFY(object->property("dataOK").toBool() == true);

    QCOMPARE(object->property("unsentException").toBool(), true);
    QCOMPARE(object->property("openedException").toBool(), true);
    QCOMPARE(object->property("sentException").toBool(), true);
    QCOMPARE(object->property("headersReceived").toBool(), true);
    QCOMPARE(object->property("loading").toBool(), true);
    QCOMPARE(object->property("done").toBool(), true);
    QCOMPARE(object->property("resetException").toBool(), true);
}

void tst_qqmlxmlhttprequest::statusText_data()
{
    QTest::addColumn<QUrl>("replyUrl");
    QTest::addColumn<QString>("statusText");

    QTest::newRow("OK") << testFileUrl("status.200.reply") << "OK";
    QTest::newRow("Not Found") << testFileUrl("status.404.reply") << "Document not found";
    QTest::newRow("Bad Request") << testFileUrl("status.400.reply") << "Bad request";
}

void tst_qqmlxmlhttprequest::responseText()
{
    QFETCH(QUrl, replyUrl);
    QFETCH(QUrl, bodyUrl);
    QFETCH(QString, responseText);

    TestHTTPServer server;
    QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
    QVERIFY(server.wait(testFileUrl("status.expect"),
                        replyUrl,
                        bodyUrl));

    QQmlComponent component(&engine, testFileUrl("responseText.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    object->setProperty("url", "http://127.0.0.1:14445/testdocument.html");
    object->setProperty("expectedText", responseText);
    component.completeCreate();

    QTRY_VERIFY(object->property("dataOK").toBool() == true);

    QCOMPARE(object->property("unsent").toBool(), true);
    QCOMPARE(object->property("opened").toBool(), true);
    QCOMPARE(object->property("sent").toBool(), true);
    QCOMPARE(object->property("headersReceived").toBool(), true);
    QCOMPARE(object->property("loading").toBool(), true);
    QCOMPARE(object->property("done").toBool(), true);
    QCOMPARE(object->property("reset").toBool(), true);
}

void tst_qqmlxmlhttprequest::responseText_data()
{
    QTest::addColumn<QUrl>("replyUrl");
    QTest::addColumn<QUrl>("bodyUrl");
    QTest::addColumn<QString>("responseText");

    QTest::newRow("OK") << testFileUrl("status.200.reply") << testFileUrl("testdocument.html") << "QML Rocks!\n";
    QTest::newRow("empty body") << testFileUrl("status.200.reply") << QUrl() << "";
    QTest::newRow("Not Found") << testFileUrl("status.404.reply") << testFileUrl("testdocument.html") << "QML Rocks!\n";
    QTest::newRow("Bad Request") << testFileUrl("status.400.reply") << testFileUrl("testdocument.html") << "QML Rocks!\n";
}

void tst_qqmlxmlhttprequest::nonUtf8()
{
    QFETCH(QString, fileName);
    QFETCH(QString, responseText);
    QFETCH(QString, xmlRootNodeValue);

    QQmlComponent component(&engine, testFileUrl("utf16.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    object->setProperty("fileName", fileName);
    QMetaObject::invokeMethod(object.data(), "startRequest");

    QTRY_VERIFY(object->property("dataOK").toBool() == true);

    QCOMPARE(object->property("responseText").toString(), responseText);

    if (!xmlRootNodeValue.isEmpty()) {
        QString rootNodeValue = object->property("responseXmlRootNodeValue").toString();
        QCOMPARE(rootNodeValue, xmlRootNodeValue);
    }
}

void tst_qqmlxmlhttprequest::nonUtf8_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("responseText");
    QTest::addColumn<QString>("xmlRootNodeValue");

    QString uc;
    uc.resize(3);
    uc[0] = QChar(0x10e3);
    uc[1] = QChar(' ');
    uc[2] = QChar(0x03a3);

    QTest::newRow("responseText") << "utf16.html" << uc + '\n' << "";
    QTest::newRow("responseXML") << "utf16.xml" << "<?xml version=\"1.0\" encoding=\"UTF-16\" standalone='yes'?>\n<root>\n" + uc + "\n</root>\n" << QString('\n' + uc + '\n');
}

// Test that calling hte XMLHttpRequest methods on a non-XMLHttpRequest object
// throws an exception
void tst_qqmlxmlhttprequest::invalidMethodUsage()
{
    QQmlComponent component(&engine, testFileUrl("invalidMethodUsage.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("readyState").toBool(), true);
    QCOMPARE(object->property("status").toBool(), true);
    QCOMPARE(object->property("statusText").toBool(), true);
    QCOMPARE(object->property("responseText").toBool(), true);
    QCOMPARE(object->property("responseXML").toBool(), true);

    QCOMPARE(object->property("open").toBool(), true);
    QCOMPARE(object->property("setRequestHeader").toBool(), true);
    QCOMPARE(object->property("send").toBool(), true);
    QCOMPARE(object->property("abort").toBool(), true);
    QCOMPARE(object->property("getResponseHeader").toBool(), true);
    QCOMPARE(object->property("getAllResponseHeaders").toBool(), true);
}

// Test that XMLHttpRequest transparently redirects
void tst_qqmlxmlhttprequest::redirects()
{
    {
        TestHTTPServer server;
        QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
        server.addRedirect("redirect.html", "http://127.0.0.1:14445/redirecttarget.html");
        server.serveDirectory(dataDirectory());

        QQmlComponent component(&engine, testFileUrl("redirects.qml"));
        QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
        QVERIFY(!object.isNull());
        object->setProperty("url", "http://127.0.0.1:14445/redirect.html");
        object->setProperty("expectedText", "");
        component.completeCreate();

        QTRY_VERIFY(object->property("done").toBool() == true);
        QCOMPARE(object->property("dataOK").toBool(), true);
    }

    {
        TestHTTPServer server;
        QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
        server.addRedirect("redirect.html", "http://127.0.0.1:14445/redirectmissing.html");
        server.serveDirectory(dataDirectory());

        QQmlComponent component(&engine, testFileUrl("redirectError.qml"));
        QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
        QVERIFY(!object.isNull());
        object->setProperty("url", "http://127.0.0.1:14445/redirect.html");
        object->setProperty("expectedText", "");
        component.completeCreate();

        QTRY_VERIFY(object->property("done").toBool() == true);
        QCOMPARE(object->property("dataOK").toBool(), true);
    }

    {
        TestHTTPServer server;
        QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
        server.addRedirect("redirect.html", "http://127.0.0.1:14445/redirect.html");
        server.serveDirectory(dataDirectory());

        QQmlComponent component(&engine, testFileUrl("redirectRecur.qml"));
        QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
        QVERIFY(!object.isNull());
        object->setProperty("url", "http://127.0.0.1:14445/redirect.html");
        object->setProperty("expectedText", "");
        component.completeCreate();

        for (int ii = 0; ii < 60; ++ii) {
            if (object->property("done").toBool()) break;
            QTest::qWait(50);
        }
        QVERIFY(object->property("done").toBool() == true);

        QCOMPARE(object->property("dataOK").toBool(), true);
    }
}

void tst_qqmlxmlhttprequest::responseXML_invalid()
{
    QQmlComponent component(&engine, testFileUrl("responseXML_invalid.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QTRY_VERIFY(object->property("dataOK").toBool() == true);

    QCOMPARE(object->property("xmlNull").toBool(), true);
}

// Test the Document DOM element
void tst_qqmlxmlhttprequest::document()
{
    QQmlComponent component(&engine, testFileUrl("document.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QTRY_VERIFY(object->property("dataOK").toBool() == true);

    QCOMPARE(object->property("xmlTest").toBool(), true);
}

// Test the Element DOM element
void tst_qqmlxmlhttprequest::element()
{
    QQmlComponent component(&engine, testFileUrl("element.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QTRY_VERIFY(object->property("dataOK").toBool() == true);

    QCOMPARE(object->property("xmlTest").toBool(), true);
}

// Test the Attr DOM element
void tst_qqmlxmlhttprequest::attr()
{
    QQmlComponent component(&engine, testFileUrl("attr.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QTRY_VERIFY(object->property("dataOK").toBool() == true);

    QCOMPARE(object->property("xmlTest").toBool(), true);
}

// Test the Text DOM element
void tst_qqmlxmlhttprequest::text()
{
    QQmlComponent component(&engine, testFileUrl("text.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QTRY_VERIFY(object->property("dataOK").toBool() == true);

    QCOMPARE(object->property("xmlTest").toBool(), true);
}

// Test the CDataSection DOM element
void tst_qqmlxmlhttprequest::cdata()
{
    QQmlComponent component(&engine, testFileUrl("cdata.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QTRY_VERIFY(object->property("dataOK").toBool() == true);

    QCOMPARE(object->property("xmlTest").toBool(), true);
}

void tst_qqmlxmlhttprequest::stateChangeCallingContext()
{
#ifdef Q_OS_WIN
    QSKIP("QTBUG-26738");
#endif

    // ensure that we don't crash by attempting to evaluate
    // without a valid calling context.

    TestHTTPServer server;
    QVERIFY2(server.listen(SERVER_PORT), qPrintable(server.errorString()));
    server.serveDirectory(dataDirectory(), TestHTTPServer::Delay);

    QQmlComponent component(&engine, testFileUrl("stateChangeCallingContext.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    server.sendDelayedItem();
    QTRY_VERIFY(object->property("success").toBool() == true);
}

QTEST_MAIN(tst_qqmlxmlhttprequest)

#include "tst_qqmlxmlhttprequest.moc"
