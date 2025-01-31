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
#include <qtranslator.h>
#include <qfile.h>

class tst_QTranslator : public QObject
{
    Q_OBJECT

public:
    tst_QTranslator();
protected:
    bool eventFilter(QObject *obj, QEvent *event);
private slots:
    void initTestCase();

    void load();
    void load2();
    void threadLoad();
    void testLanguageChange();
    void plural();
    void translate_qm_file_generated_with_msgfmt();
    void loadFromResource();
    void loadDirectory();
    void dependencies();

private:
    int languageChangeEventCounter;
};

tst_QTranslator::tst_QTranslator()
    : languageChangeEventCounter(0)
{
    qApp->installEventFilter(this);
}

void tst_QTranslator::initTestCase()
{
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_NO_SDK)
    QString sourceDir(":/android_testdata/");
    QDirIterator it(sourceDir, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();

        QFileInfo sourceFileInfo = it.fileInfo();
        if (!sourceFileInfo.isDir()) {
            QFileInfo destinationFileInfo(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1Char('/') + sourceFileInfo.filePath().mid(sourceDir.length()));

            if (!destinationFileInfo.exists()) {
                QVERIFY(QDir().mkpath(destinationFileInfo.path()));
                QVERIFY(QFile::copy(sourceFileInfo.filePath(), destinationFileInfo.filePath()));
            }
        }
    }

    QDir::setCurrent(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
#endif

    // chdir into the directory containing our testdata,
    // to make the code simpler (load testdata via relative paths)
    QString testdata_dir = QFileInfo(QFINDTESTDATA("hellotr_la.qm")).absolutePath();
    QVERIFY2(QDir::setCurrent(testdata_dir), qPrintable("Could not chdir to " + testdata_dir));
}

bool tst_QTranslator::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        ++languageChangeEventCounter;
    return false;
}

void tst_QTranslator::load()
{

    QTranslator tor( 0 );
    tor.load("hellotr_la");
    QVERIFY(!tor.isEmpty());
    QCOMPARE(tor.translate("QPushButton", "Hello world!"), QString::fromLatin1("Hallo Welt!"));
}

void tst_QTranslator::load2()
{
    QTranslator tor( 0 );
    QFile file("hellotr_la.qm");
    file.open(QFile::ReadOnly);
    QByteArray data = file.readAll();
    tor.load((const uchar *)data.constData(), data.length());
    QVERIFY(!tor.isEmpty());
    QCOMPARE(tor.translate("QPushButton", "Hello world!"), QString::fromLatin1("Hallo Welt!"));
}

class TranslatorThread : public QThread
{
    void run() {
        QTranslator tor( 0 );
        tor.load("hellotr_la");

        if (tor.isEmpty())
            qFatal("Could not load translation");
        if (tor.translate("QPushButton", "Hello world!") !=  QString::fromLatin1("Hallo Welt!"))
            qFatal("Test string was not translated correctlys");
    }
};


void tst_QTranslator::threadLoad()
{
    TranslatorThread thread;
    thread.start();
    QVERIFY(thread.wait(10 * 1000));
}

void tst_QTranslator::testLanguageChange()
{
    languageChangeEventCounter = 0;

    QTranslator *tor = new QTranslator;
    tor->load("hellotr_la.qm");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 0);

    tor->load("doesn't exist, same as clearing");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 0);

    tor->load("hellotr_la.qm");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 0);

    qApp->installTranslator(tor);
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 1);

    tor->load("doesn't exist, same as clearing");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 2);

    tor->load("hellotr_la.qm");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 3);

    qApp->removeTranslator(tor);
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 4);

    tor->load("doesn't exist, same as clearing");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 4);

    qApp->installTranslator(tor);
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 4);

    tor->load("hellotr_la.qm");
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 5);

    delete tor;
    tor = 0;
    qApp->sendPostedEvents();
    qApp->sendPostedEvents();
    QCOMPARE(languageChangeEventCounter, 6);
}


void tst_QTranslator::plural()
{

    QTranslator tor( 0 );
    tor.load("hellotr_la");
    QVERIFY(!tor.isEmpty());
    QCoreApplication::installTranslator(&tor);
    QCOMPARE(QCoreApplication::translate("QPushButton", "Hello %n world(s)!", 0, 0), QString::fromLatin1("Hallo 0 Welten!"));
    QCOMPARE(QCoreApplication::translate("QPushButton", "Hello %n world(s)!", 0, 1), QString::fromLatin1("Hallo 1 Welt!"));
    QCOMPARE(QCoreApplication::translate("QPushButton", "Hello %n world(s)!", 0, 2), QString::fromLatin1("Hallo 2 Welten!"));
}

void tst_QTranslator::translate_qm_file_generated_with_msgfmt()
{
    QTranslator translator;
    translator.load("msgfmt_from_po");
    qApp->installTranslator(&translator);

    QCOMPARE(QCoreApplication::translate("", "Intro"), QLatin1String("Einleitung"));
    // The file is converted from a po file, thus it does not have any context info.
    // The following should then not be translated
    QCOMPARE(QCoreApplication::translate("contekst", "Intro"), QLatin1String("Intro"));
    QCOMPARE(QCoreApplication::translate("contekst", "Intro\0\0"), QLatin1String("Intro"));
    QCOMPARE(QCoreApplication::translate("contekst", "Intro\0x"), QLatin1String("Intro"));
    QCOMPARE(QCoreApplication::translate("", "Intro\0\0"), QLatin1String("Einleitung"));
    QCOMPARE(QCoreApplication::translate("", "Intro\0x"), QLatin1String("Einleitung"));

    qApp->removeTranslator(&translator);
}

void tst_QTranslator::loadFromResource()
{
    QTranslator tor;
    tor.load(":/tst_qtranslator/hellotr_la.qm");
    QVERIFY(!tor.isEmpty());
    QCOMPARE(tor.translate("QPushButton", "Hello world!"), QString::fromLatin1("Hallo Welt!"));
}

void tst_QTranslator::loadDirectory()
{
    QString current_base = QDir::current().dirName();
    QVERIFY(QFileInfo("../" + current_base).isDir());

    QTranslator tor;
    tor.load(current_base, "..");
    QVERIFY(tor.isEmpty());
}

void tst_QTranslator::dependencies()
{
    {
        // load
        QTranslator tor;
        tor.load("dependencies_la");
        QVERIFY(!tor.isEmpty());
        QCOMPARE(tor.translate("QPushButton", "Hello world!"), QString::fromLatin1("Hallo Welt!"));

        // plural
        QCoreApplication::installTranslator(&tor);
        QCOMPARE(QCoreApplication::translate("QPushButton", "Hello %n world(s)!", 0, 0), QString::fromLatin1("Hallo 0 Welten!"));
        QCOMPARE(QCoreApplication::translate("QPushButton", "Hello %n world(s)!", 0, 1), QString::fromLatin1("Hallo 1 Welt!"));
        QCOMPARE(QCoreApplication::translate("QPushButton", "Hello %n world(s)!", 0, 2), QString::fromLatin1("Hallo 2 Welten!"));

        // pick up translation from the file with dependencies
        QCOMPARE(tor.translate("QPushButton", "It's a small world"), QString::fromLatin1("Es ist eine kleine Welt"));
    }

    {
        QTranslator tor( 0 );
        QFile file("dependencies_la.qm");
        file.open(QFile::ReadOnly);
        QByteArray data = file.readAll();
        tor.load((const uchar *)data.constData(), data.length());
        QVERIFY(!tor.isEmpty());
        QCOMPARE(tor.translate("QPushButton", "Hello world!"), QString::fromLatin1("Hallo Welt!"));
    }
}


QTEST_MAIN(tst_QTranslator)
#include "tst_qtranslator.moc"
