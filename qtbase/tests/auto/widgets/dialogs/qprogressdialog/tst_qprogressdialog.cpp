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


#include <QtTest/QtTest>

#include <qapplication.h>
#include <qdebug.h>
#include <qprogressbar.h>
#include <qprogressdialog.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qpointer.h>
#include <qthread.h>
#include <qtranslator.h>

class tst_QProgressDialog : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cleanup();
    void autoShow_data();
    void autoShow();
    void getSetCheck();
    void task198202();
    void QTBUG_31046();
    void settingCustomWidgets();
    void i18n();
};

void tst_QProgressDialog::cleanup()
{
    QVERIFY(QApplication::topLevelWindows().empty());
}

void tst_QProgressDialog::autoShow_data()
{
    QTest::addColumn<int>("min");
    QTest::addColumn<int>("max");
    QTest::addColumn<int>("delay");
    QTest::addColumn<bool>("expectedAutoShow");

    QTest::newRow("50_to_100_long") << 50 << 100 << 100 << true; // 50*100ms = 5s
    QTest::newRow("50_to_100_short") << 50 << 1 << 100 << false; // 50*1ms = 50ms

    QTest::newRow("0_to_100_long") << 0 << 100 << 100 << true; // 100*100ms = 10s
    QTest::newRow("0_to_10_short") << 0 << 10 << 100 << false; // 10*100ms = 1s
}

void tst_QProgressDialog::autoShow()
{
    QFETCH(int, min);
    QFETCH(int, max);
    QFETCH(int, delay);
    QFETCH(bool, expectedAutoShow);

    QProgressDialog dlg("", "", min, max);
    dlg.setValue(0);
    QThread::msleep(delay);
    dlg.setValue(min+1);
    QCOMPARE(dlg.isVisible(), expectedAutoShow);
}

// Testing get/set functions
void tst_QProgressDialog::getSetCheck()
{
    QProgressDialog obj1;
    // bool QProgressDialog::autoReset()
    // void QProgressDialog::setAutoReset(bool)
    obj1.setAutoReset(false);
    QCOMPARE(false, obj1.autoReset());
    obj1.setAutoReset(true);
    QCOMPARE(true, obj1.autoReset());

    // bool QProgressDialog::autoClose()
    // void QProgressDialog::setAutoClose(bool)
    obj1.setAutoClose(false);
    QCOMPARE(false, obj1.autoClose());
    obj1.setAutoClose(true);
    QCOMPARE(true, obj1.autoClose());

    // int QProgressDialog::maximum()
    // void QProgressDialog::setMaximum(int)
    obj1.setMaximum(0);
    QCOMPARE(0, obj1.maximum());
    obj1.setMaximum(INT_MIN);
    QCOMPARE(INT_MIN, obj1.maximum());
    obj1.setMaximum(INT_MAX);
    QCOMPARE(INT_MAX, obj1.maximum());

    // int QProgressDialog::minimum()
    // void QProgressDialog::setMinimum(int)
    obj1.setMinimum(0);
    QCOMPARE(0, obj1.minimum());
    obj1.setMinimum(INT_MIN);
    QCOMPARE(INT_MIN, obj1.minimum());
    obj1.setMinimum(INT_MAX);
    QCOMPARE(INT_MAX, obj1.minimum());

    // int QProgressDialog::value()
    // void QProgressDialog::setValue(int)
    obj1.setMaximum(INT_MAX);
    obj1.setMinimum(INT_MIN);
    obj1.setValue(0);
    QCOMPARE(0, obj1.value());
    obj1.setValue(INT_MIN+1);
    QCOMPARE(INT_MIN+1, obj1.value());
    obj1.setValue(INT_MIN);
    QCOMPARE(INT_MIN, obj1.value());
    obj1.setValue(INT_MAX-1);
    QCOMPARE(INT_MAX-1, obj1.value());

    obj1.setValue(INT_MAX);
    QCOMPARE(INT_MIN, obj1.value()); // We set autoReset, the thing is reset

    obj1.setAutoReset(false);
    obj1.setValue(INT_MAX);
    QCOMPARE(INT_MAX, obj1.value());
    obj1.setAutoReset(true);

    // int QProgressDialog::minimumDuration()
    // void QProgressDialog::setMinimumDuration(int)
    obj1.setMinimumDuration(0);
    QCOMPARE(0, obj1.minimumDuration());
    obj1.setMinimumDuration(INT_MIN);
    QCOMPARE(INT_MIN, obj1.minimumDuration());
    obj1.setMinimumDuration(INT_MAX);
    QCOMPARE(INT_MAX, obj1.minimumDuration());
}

void tst_QProgressDialog::task198202()
{
    //should not crash
    QProgressDialog dlg(QLatin1String("test"),QLatin1String("test"),1,10);
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg));
    int futureHeight = dlg.sizeHint().height() - dlg.findChild<QLabel*>()->sizeHint().height();
    dlg.setLabel(0);
    QTest::ignoreMessage(QtWarningMsg, "QProgressDialog::setBar: Cannot set a null progress bar");
    dlg.setBar(0);
    QTest::qWait(20);
    QCOMPARE(dlg.sizeHint().height(), futureHeight);
}

void tst_QProgressDialog::QTBUG_31046()
{
    QProgressDialog dlg("", "", 50, 60);
    dlg.setValue(0);
    QThread::msleep(200);
    dlg.setValue(50);
    QCOMPARE(50, dlg.value());
}

void tst_QProgressDialog::settingCustomWidgets()
{
    QPointer<QLabel> l = new QLabel;
    QPointer<QPushButton> btn = new QPushButton;
    QPointer<QProgressBar> bar = new QProgressBar;
    QVERIFY(!l->parent());
    QVERIFY(!btn->parent());
    QVERIFY(!bar->parent());

    {
        QProgressDialog dlg;

        QVERIFY(!dlg.isAncestorOf(l));
        dlg.setLabel(l);
        QVERIFY(dlg.isAncestorOf(l));
        QTest::ignoreMessage(QtWarningMsg, "QProgressDialog::setLabel: Attempt to set the same label again");
        dlg.setLabel(l);          // setting the same widget again should not crash
        QVERIFY(l);               // and not delete the (old == new) widget

        QVERIFY(!dlg.isAncestorOf(btn));
        dlg.setCancelButton(btn);
        QVERIFY(dlg.isAncestorOf(btn));
        QTest::ignoreMessage(QtWarningMsg, "QProgressDialog::setCancelButton: Attempt to set the same button again");
        dlg.setCancelButton(btn); // setting the same widget again should not crash
        QVERIFY(btn);             // and not delete the (old == new) widget

        QVERIFY(!dlg.isAncestorOf(bar));
        dlg.setBar(bar);
        QVERIFY(dlg.isAncestorOf(bar));
        QTest::ignoreMessage(QtWarningMsg, "QProgressDialog::setBar: Attempt to set the same progress bar again");
        dlg.setBar(bar);          // setting the same widget again should not crash
        QVERIFY(bar);             // and not delete the (old == new) widget
    }

    QVERIFY(!l);
    QVERIFY(!btn);
    QVERIFY(!bar);
}

class QTestTranslator : public QTranslator
{
    const QString m_str;
public:
    explicit QTestTranslator(QString str) : m_str(qMove(str)) {}

    QString translate(const char *, const char *sourceText, const char *, int) const Q_DECL_OVERRIDE
    { return m_str + sourceText + m_str; }

    bool isEmpty() const Q_DECL_OVERRIDE { return false; }
};

template <typename Translator>
class QTranslatorGuard {
    Translator t;
public:
    template <typename Arg>
    explicit QTranslatorGuard(Arg a) : t(qMove(a))
    { qApp->installTranslator(&t); }
    ~QTranslatorGuard()
    { qApp->removeTranslator(&t); }
};

void tst_QProgressDialog::i18n()
{
    QProgressDialog dlg;
    QPushButton *btn = dlg.findChild<QPushButton*>();
    QVERIFY(btn);
    const QString xxx = QStringLiteral("xxx");
    {
        QTranslatorGuard<QTestTranslator> guard(xxx);
        {
            QPushButton *btn = dlg.findChild<QPushButton*>();
            QVERIFY(btn);
            QTRY_COMPARE(btn->text(), QProgressDialog::tr("Cancel"));
            QVERIFY(btn->text().startsWith(xxx));
        }
    }
    QVERIFY(btn);
    QTRY_COMPARE(btn->text(), QProgressDialog::tr("Cancel"));
    QVERIFY(!btn->text().startsWith(xxx));
}

QTEST_MAIN(tst_QProgressDialog)
#include "tst_qprogressdialog.moc"
