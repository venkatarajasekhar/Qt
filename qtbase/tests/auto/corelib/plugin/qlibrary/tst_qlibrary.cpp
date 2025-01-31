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
#include <qdir.h>
#include <qlibrary.h>
#include <QtCore/QRegExp>


// Helper macros to let us know if some suffixes and prefixes are valid
#define bundle_VALID    false
#define dylib_VALID     false
#define sl_VALID        false
#define a_VALID         false
#define so_VALID        false
#define dll_VALID       false
#define DLL_VALID       false

#if defined(Q_OS_DARWIN)
# undef bundle_VALID
# undef dylib_VALID
# undef so_VALID
# define bundle_VALID   true
# define dylib_VALID    true
# define so_VALID       true
# define SUFFIX         ".dylib"
# define PREFIX         "lib"

#elif defined(Q_OS_HPUX)
# undef sl_VALID
# define sl_VALID       true
#  ifndef __ia64
#   define SUFFIX         ".sl"
#   define PREFIX         "lib"
#  else
#   undef so_VALID
#   define so_VALID       true
#   define SUFFIX         ".so"
#   define PREFIX         "lib"
#  endif

#elif defined(Q_OS_AIX)
# undef a_VALID
# undef so_VALID
# define a_VALID        true
# define so_VALID       true
# define SUFFIX         ".a"
# define PREFIX         "lib"

#elif defined(Q_OS_WIN)
# undef dll_VALID
# define dll_VALID      true
# undef DLL_VALID
# define DLL_VALID      true
# define SUFFIX         ".dll"
# define PREFIX         ""

#else  // all other Unix
# undef so_VALID
# define so_VALID       true
# define SUFFIX         ".so"
# define PREFIX         "lib"
#endif

static QString sys_qualifiedLibraryName(const QString &fileName)
{
    QString appDir = QCoreApplication::applicationDirPath();
    return appDir + "/" + PREFIX + fileName + SUFFIX;
}

QT_FORWARD_DECLARE_CLASS(QLibrary)
class tst_QLibrary : public QObject
{
    Q_OBJECT

enum QLibraryOperation {
    Load = 1,
    Unload = 2,
    Resolve = 3,
    OperationMask = 7,
    DontSetFileName = 0x100
};
private slots:
    void initTestCase();

    void load();
    void load_data();
    void library_data();
    void resolve_data();
    void resolve();
    void unload_data();
    void unload();
    void unload_after_implicit_load();
    void isLibrary_data();
    void isLibrary();
    void version_data();
    void version();
    void errorString_data();
    void errorString();
    void loadHints();
    void loadHints_data();
    void fileName_data();
    void fileName();
    void multipleInstancesForOneLibrary();
};

typedef int (*VersionFunction)(void);

void tst_QLibrary::initTestCase()
{
    // chdir to our testdata directory, and use relative paths in some tests.
    QString testdatadir = QFileInfo(QFINDTESTDATA("library_path")).absolutePath();
    QVERIFY2(QDir::setCurrent(testdatadir), qPrintable("Could not chdir to " + testdatadir));
}

void tst_QLibrary::version_data()
{
    QTest::addColumn<QString>("lib");
    QTest::addColumn<int>("loadversion");
    QTest::addColumn<int>("resultversion");

    QTest::newRow( "ok00, version 1" ) << "mylib" << 1 << 1;
    QTest::newRow( "ok00, version 2" ) << "mylib" << 2 << 2;
    QTest::newRow( "ok00, default to last version" ) << "mylib" << -1 << 2;
}

void tst_QLibrary::version()
{
    QFETCH( QString, lib );
    QFETCH( int, loadversion );
    QFETCH( int, resultversion );

#if !defined(Q_OS_AIX) && !defined(Q_OS_WIN)
    QString appDir = QCoreApplication::applicationDirPath();
    QLibrary library( appDir + QLatin1Char('/') + lib, loadversion );
    QVERIFY2(library.load(), qPrintable(library.errorString()));

    VersionFunction fnVersion = (VersionFunction)library.resolve("mylibversion");
    QVERIFY(fnVersion);
    QCOMPARE(fnVersion(), resultversion);
    QVERIFY2(library.unload(), qPrintable(library.errorString()));
#else
    Q_UNUSED(lib);
    Q_UNUSED(loadversion);
    Q_UNUSED(resultversion);
#endif
}

void tst_QLibrary::load_data()
{
    QTest::addColumn<QString>("lib");
    QTest::addColumn<bool>("result");

    QString appDir = QCoreApplication::applicationDirPath();

    QTest::newRow( "ok00" ) << appDir + "/mylib" << true;
    QTest::newRow( "notexist" ) << appDir + "/nolib" << false;
    QTest::newRow( "badlibrary" ) << appDir + "/qlibrary.pro" << false;

#ifdef Q_OS_MAC
    QTest::newRow("ok (libmylib ver. 1)") << appDir + "/libmylib" <<true;
#endif

# if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    QTest::newRow( "ok01 (with suffix)" ) << appDir + "/mylib.dll" << true;
    QTest::newRow( "ok02 (with non-standard suffix)" ) << appDir + "/mylib.dl2" << true;
    QTest::newRow( "ok03 (with many dots)" ) << appDir + "/system.qt.test.mylib.dll" << true;
# elif defined Q_OS_UNIX
    QTest::newRow( "ok01 (with suffix)" ) << appDir + "/libmylib" SUFFIX << true;
    QTest::newRow( "ok02 (with non-standard suffix)" ) << appDir + "/libmylib.so2" << true;
    QTest::newRow( "ok03 (with many dots)" ) << appDir + "/system.qt.test.mylib.so" << true;
# endif  // Q_OS_UNIX
}

void tst_QLibrary::load()
{
    QFETCH( QString, lib );
    QFETCH( bool, result );

    QLibrary library( lib );
    bool ok = library.load();
    if ( result ) {
        QVERIFY2( ok, qPrintable(library.errorString()) );
        QVERIFY2( library.unload(), qPrintable(library.errorString()) );
    } else {
        QVERIFY( !ok );
    }
}

void tst_QLibrary::unload_data()
{
    QTest::addColumn<QString>("lib");
    QTest::addColumn<bool>("result");

    QString appDir = QCoreApplication::applicationDirPath();

    QTest::newRow( "mylib" ) << appDir + "/mylib" << true;
#ifdef Q_OS_MAC
    if (QSysInfo::MacintoshVersion <= QSysInfo::MV_10_3)
        QEXPECT_FAIL("mylib", "dlcompat cannot unload libraries", Continue);
#endif
    QTest::newRow( "ok01" ) << appDir + "/nolib" << false;
}

void tst_QLibrary::unload()
{
    QFETCH( QString, lib );
    QFETCH( bool, result );

    QLibrary library( lib );
    library.load();
    bool ok = library.unload();
    if ( result ) {
        QVERIFY2( ok, qPrintable(library.errorString()) );
    } else {
        QVERIFY( !ok );
    }
}

void tst_QLibrary::unload_after_implicit_load()
{
    QLibrary library( QCoreApplication::applicationDirPath() + "/mylib" );
    QFunctionPointer p = library.resolve("mylibversion");
    QVERIFY(p); // Check if it was loaded
    QVERIFY(library.isLoaded());
    QVERIFY(library.unload());
    QCOMPARE(library.isLoaded(), false);
}

void tst_QLibrary::resolve_data()
{
    QTest::addColumn<QString>("lib");
    QTest::addColumn<QString>("symbol");
    QTest::addColumn<bool>("goodPointer");

    QString appDir = QCoreApplication::applicationDirPath();

    QTest::newRow( "ok00" ) << appDir + "/mylib" << QString("mylibversion") << true;
    QTest::newRow( "bad00" ) << appDir + "/mylib" << QString("nosym") << false;
    QTest::newRow( "bad01" ) << appDir + "/nolib" << QString("nosym") << false;
}

void tst_QLibrary::resolve()
{
    typedef int (*testFunc)();
    QFETCH( QString, lib );
    QFETCH( QString, symbol );
    QFETCH( bool, goodPointer );

    QLibrary library( lib );
    testFunc func = (testFunc) library.resolve( symbol.toLatin1() );
    if ( goodPointer ) {
        QVERIFY( func != 0 );
    } else {
        QVERIFY( func == 0 );
    }
    library.unload();
}

void tst_QLibrary::library_data()
{
    QTest::addColumn<QString>("lib");
}

void tst_QLibrary::isLibrary_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<bool>("valid");

    // use the macros #defined at the top of the file
    QTest::newRow("bad") << QString("mylib.bad") << false;
    QTest::newRow(".a") << QString("mylib.a") << a_VALID;
    QTest::newRow(".bundle") << QString("mylib.bundle") << bundle_VALID;
    QTest::newRow(".dll") << QString("mylib.dll") << dll_VALID;
    QTest::newRow(".DLL") << QString("MYLIB.DLL") << DLL_VALID;
    QTest::newRow(".dl2" ) << QString("mylib.dl2") << false;
    QTest::newRow(".dylib") << QString("mylib.dylib") << dylib_VALID;
    QTest::newRow(".sl") << QString("mylib.sl") << sl_VALID;
    QTest::newRow(".so") << QString("mylib.so") << so_VALID;
    QTest::newRow(".so+version") << QString("mylib.so.0") << so_VALID;

    // special tests:
#ifndef Q_OS_MAC
    QTest::newRow("version+.so") << QString("libc-2.7.so") << so_VALID;
    QTest::newRow("version+.so+version") << QString("liboil-0.3.so.0.1.0") << so_VALID;
#else
    QTest::newRow("version+.so") << QString("libc-2.7.so") << false;
    QTest::newRow("version+.so+version") << QString("liboil-0.3.so.0.1.0") << false;
#endif
#ifdef Q_OS_MAC
    QTest::newRow("good (libmylib.1.0.0.dylib)") << QString("libmylib.1.0.0.dylib") << true;
    QTest::newRow("good (libmylib.dylib)") << QString("libmylib.dylib") << true;
    QTest::newRow("good (libmylib.so)") << QString("libmylib.so") << true;
    QTest::newRow("good (libmylib.so.1.0.0)") << QString("libmylib.so.1.0.0") << true;

    QTest::newRow("bad (libmylib.1.0.0.foo)") << QString("libmylib.1.0.0.foo") << false;
#elif defined(Q_OS_WIN)
    QTest::newRow("good (with many dots)" ) << "/system.qt.test.mylib.dll" << true;
#endif
}

void tst_QLibrary::isLibrary()
{
    QFETCH( QString, filename );
    QFETCH( bool, valid );

    QCOMPARE(QLibrary::isLibrary(filename), valid);
}

void tst_QLibrary::errorString_data()
{
    QTest::addColumn<int>("operation");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<bool>("success");
    QTest::addColumn<QString>("errorString");

    QString appDir = QCoreApplication::applicationDirPath();

    QTest::newRow("bad load()") << (int)Load << QString("nosuchlib") << false << QString("Cannot load library nosuchlib: .*");
    QTest::newRow("call errorString() on QLibrary with no d-pointer (crashtest)") << (int)(Load | DontSetFileName) << QString() << false << QString("Unknown error");
#ifdef Q_OS_WINCE
    QTest::newRow("bad resolve") << (int)Resolve << appDir + "/mylib" << false << QString("Cannot resolve symbol \"nosuchsymbol\" in .*: .*");
#else
    QTest::newRow("bad resolve") << (int)Resolve << appDir + "/mylib" << false << QString("Cannot resolve symbol \"nosuchsymbol\" in \\S+: .*");
#endif
    QTest::newRow("good resolve") << (int)Resolve << appDir + "/mylib" << true << QString("Unknown error");

#ifdef Q_OS_WIN
    QTest::newRow("bad load() with .dll suffix") << (int)Load << QString("nosuchlib.dll") << false << QString("Cannot load library nosuchlib.dll: The specified module could not be found.");
//    QTest::newRow("bad unload") << (int)Unload << QString("nosuchlib.dll") << false << QString("QLibrary::unload_sys: Cannot unload nosuchlib.dll (The specified module could not be found.)");
#elif defined Q_OS_MAC
#else
    QTest::newRow("load invalid file") << (int)Load << QFINDTESTDATA("library_path/invalid.so") << false << QString("Cannot load library.*");
#endif
}

void tst_QLibrary::errorString()
{
    QFETCH(int, operation);
    QFETCH(QString, fileName);
    QFETCH(bool, success);
    QFETCH(QString, errorString);

    QLibrary lib;
    if (!(operation & DontSetFileName)) {
        lib.setFileName(fileName);
    }

    bool ok = false;
    switch (operation & OperationMask) {
        case Load:
            ok = lib.load();
            break;
        case Unload:
            ok = lib.load();    //###
            ok = lib.unload();
            break;
        case Resolve: {
            ok = lib.load();
            QCOMPARE(ok, true);
            if (success) {
                ok = lib.resolve("mylibversion");
            } else {
                ok = lib.resolve("nosuchsymbol");
            }
            break;}
        default:
            QFAIL(qPrintable(QString("Unknown operation: %1").arg(operation)));
            break;
    }
    QRegExp re(errorString);
    QString libErrorString = lib.errorString();
    QVERIFY(!lib.isLoaded() || lib.unload());
    QVERIFY2(re.exactMatch(libErrorString), qPrintable(libErrorString));
    QCOMPARE(ok, success);
}

void tst_QLibrary::loadHints_data()
{
    QTest::addColumn<QString>("lib");
    QTest::addColumn<int>("loadHints");
    QTest::addColumn<bool>("result");

    QLibrary::LoadHints lh;
#if defined(Q_OS_AIX)
    if (QFile::exists("/usr/lib/libGL.a") || QFile::exists("/usr/X11R6/lib/libGL.a")) {
# if QT_POINTER_SIZE == 4
        QTest::newRow( "ok03 (Archive member)" ) << "libGL.a(shr.o)" << int(QLibrary::LoadArchiveMemberHint) << true;
# else
        QTest::newRow( "ok03 (Archive member)" ) << "libGL.a(shr_64.o)" << int(QLibrary::LoadArchiveMemberHint) << true;
#endif
    }
#endif

    QString appDir = QCoreApplication::applicationDirPath();

    lh |= QLibrary::ResolveAllSymbolsHint;
# if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    QTest::newRow( "ok01 (with suffix)" ) << appDir + "/mylib.dll" << int(lh) << true;
    QTest::newRow( "ok02 (with non-standard suffix)" ) << appDir + "/mylib.dl2" << int(lh) << true;
    QTest::newRow( "ok03 (with many dots)" ) << appDir + "/system.qt.test.mylib.dll" << int(lh) << true;
# elif defined Q_OS_UNIX
    QTest::newRow( "ok01 (with suffix)" ) << appDir + "/libmylib" SUFFIX << int(lh) << true;
    QTest::newRow( "ok02 (with non-standard suffix)" ) << appDir + "/libmylib.so2" << int(lh) << true;
    QTest::newRow( "ok03 (with many dots)" ) << appDir + "/system.qt.test.mylib.so" << int(lh) << true;
# endif  // Q_OS_UNIX
}

void tst_QLibrary::loadHints()
{
    QFETCH( QString, lib );
    QFETCH( int, loadHints);
    QFETCH( bool, result );
    //QLibrary library( lib );
    QLibrary library;
    QLibrary::LoadHints lh(loadHints);
    if (int(loadHints) != 0) {
        lh |= library.loadHints();
        library.setLoadHints(lh);

        // confirm that another QLibrary doesn't get affected - QTBUG-39642
        QCOMPARE(QLibrary().loadHints(), QLibrary::LoadHints());
    }
    library.setFileName(lib);
    QCOMPARE(library.loadHints(), lh);
    bool ok = library.load();

    // we can't change the hints anymore
    library.setLoadHints(QLibrary::LoadHints());
    QCOMPARE(library.loadHints(), lh);

    // confirm that a new QLibrary inherits the hints too
    QCOMPARE(QLibrary(lib).loadHints(), lh);

    if ( result ) {
        QVERIFY( ok );
        QVERIFY(library.unload());
    } else {
        QVERIFY( !ok );
    }
}

void tst_QLibrary::fileName_data()
{
    QTest::addColumn<QString>("libName");
    QTest::addColumn<QString>("expectedFilename");

    QTest::newRow( "ok02" ) << sys_qualifiedLibraryName(QLatin1String("mylib"))
                            << sys_qualifiedLibraryName(QLatin1String("mylib"));
#ifdef Q_OS_WIN
#ifndef Q_OS_WINCE
    QTest::newRow( "ok03" ) << "user32"
                            << "USER32.dll";
#else
    QTest::newRow( "ok03" ) << "coredll"
                            << "coredll.dll";
#endif
#endif
}

void tst_QLibrary::fileName()
{
    QFETCH( QString, libName);
    QFETCH( QString, expectedFilename);

    QLibrary lib(libName);
    bool ok = lib.load();
    QVERIFY2(ok, qPrintable(lib.errorString()));
#if defined(Q_OS_WIN)
    QCOMPARE(lib.fileName().toLower(), expectedFilename.toLower());
#else
    QCOMPARE(lib.fileName(), expectedFilename);
#endif
    QVERIFY(lib.unload());
}

void tst_QLibrary::multipleInstancesForOneLibrary()
{
    QString lib = QCoreApplication::applicationDirPath() + "/mylib";

    {
        QLibrary lib1(lib);
        QLibrary lib2(lib);
        QCOMPARE(lib1.isLoaded(), false);
        QCOMPARE(lib2.isLoaded(), false);
        lib1.load();
        QCOMPARE(lib1.isLoaded(), true);
        QCOMPARE(lib2.isLoaded(), true);
        QCOMPARE(lib1.unload(), true);
        QCOMPARE(lib1.isLoaded(), false);
        QCOMPARE(lib2.isLoaded(), false);
        lib1.load();
        lib2.load();
        QCOMPARE(lib1.isLoaded(), true);
        QCOMPARE(lib2.isLoaded(), true);
        QCOMPARE(lib1.unload(), false);
        QCOMPARE(lib1.isLoaded(), true);
        QCOMPARE(lib2.isLoaded(), true);
        QCOMPARE(lib2.unload(), true);
        QCOMPARE(lib1.isLoaded(), false);
        QCOMPARE(lib2.isLoaded(), false);

        // Finally; unload on that is already unloaded
        QCOMPARE(lib1.unload(), false);
    }

    //now let's try with a 3rd one that will go out of scope
    {
        QLibrary lib1(lib);
        QCOMPARE(lib1.isLoaded(), false);
        lib1.load();
        QCOMPARE(lib1.isLoaded(), true);
    }
    QLibrary lib2(lib);
    //lib2 should be loaded because lib1 was loaded and never unloaded
    QCOMPARE(lib2.isLoaded(), true);
}

QTEST_MAIN(tst_QLibrary)
#include "tst_qlibrary.moc"
