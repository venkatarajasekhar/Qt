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

#include <qrasterwindow.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>
#include <QtGui/QPainter>

#include <QtTest/QtTest>

#include <QEvent>
#include <QStyleHints>

#if defined(Q_OS_QNX)
#include <QOpenGLContext>
#elif defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
#  include <QtCore/qt_windows.h>
#endif

// For QSignalSpy slot connections.
Q_DECLARE_METATYPE(Qt::ScreenOrientation)
Q_DECLARE_METATYPE(QWindow::Visibility)

class tst_QWindow: public QObject
{
    Q_OBJECT

private slots:
    void eventOrderOnShow();
    void resizeEventAfterResize();
    void mapGlobal();
    void positioning_data();
    void positioning();
    void positioningDuringMinimized();
    void isExposed();
    void isActive();
    void testInputEvents();
    void touchToMouseTranslation();
    void mouseToTouchTranslation();
    void mouseToTouchLoop();
    void touchCancel();
    void touchCancelWithTouchToMouse();
    void touchInterruptedByPopup();
    void orientation();
    void sizes();
    void close();
    void activateAndClose();
    void mouseEventSequence();
    void windowModality();
    void inputReentrancy();
    void tabletEvents();
    void windowModality_QTBUG27039();
    void visibility();
    void mask();
    void initialSize();
    void modalDialog();
    void modalDialogClosingOneOfTwoModal();
    void modalWithChildWindow();
    void modalWindowModallity();
    void modalWindowPosition();
    void windowsTransientChildren();
    void initTestCase();
    void cleanup();

private:
    QPoint m_availableTopLeft;
    QSize m_testWindowSize;
    QTouchDevice *touchDevice;
};

void tst_QWindow::initTestCase()
{
    // Size of reference window, 200 for < 2000, scale up for larger screens
    // to avoid Windows warnings about minimum size for decorated windows.
    int width = 200;
    const QScreen *screen = QGuiApplication::primaryScreen();
    m_availableTopLeft = screen->availableGeometry().topLeft();
    const int screenWidth = screen->geometry().width();
    if (screenWidth > 2000)
        width = 100 * ((screenWidth + 500) / 1000);
    m_testWindowSize = QSize(width, width);
    touchDevice = new QTouchDevice;
    touchDevice->setType(QTouchDevice::TouchScreen);
    QWindowSystemInterface::registerTouchDevice(touchDevice);
}

void tst_QWindow::cleanup()
{
    QVERIFY(QGuiApplication::allWindows().isEmpty());
}

void tst_QWindow::mapGlobal()
{
    QWindow a;
    QWindow b(&a);
    QWindow c(&b);

    a.setGeometry(10, 10, 300, 300);
    b.setGeometry(20, 20, 200, 200);
    c.setGeometry(40, 40, 100, 100);

    QCOMPARE(a.mapToGlobal(QPoint(100, 100)), QPoint(110, 110));
    QCOMPARE(b.mapToGlobal(QPoint(100, 100)), QPoint(130, 130));
    QCOMPARE(c.mapToGlobal(QPoint(100, 100)), QPoint(170, 170));

    QCOMPARE(a.mapFromGlobal(QPoint(100, 100)), QPoint(90, 90));
    QCOMPARE(b.mapFromGlobal(QPoint(100, 100)), QPoint(70, 70));
    QCOMPARE(c.mapFromGlobal(QPoint(100, 100)), QPoint(30, 30));
}

class Window : public QWindow
{
public:
    Window(const Qt::WindowFlags flags = Qt::Window | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
    {
        reset();
        setFlags(flags);
#if defined(Q_OS_QNX)
        setSurfaceType(QSurface::OpenGLSurface);
#endif
    }

    void reset()
    {
        m_received.clear();
        m_framePositionsOnMove.clear();
    }

    bool event(QEvent *event)
    {
        m_received[event->type()]++;
        m_order << event->type();
        if (event->type() == QEvent::Expose)
            m_exposeRegion = static_cast<QExposeEvent *>(event)->region();
        else if (event->type() == QEvent::Move)
            m_framePositionsOnMove << framePosition();

        return QWindow::event(event);
    }

    int received(QEvent::Type type)
    {
        return m_received.value(type, 0);
    }

    int eventIndex(QEvent::Type type)
    {
        return m_order.indexOf(type);
    }

    QRegion exposeRegion() const
    {
        return m_exposeRegion;
    }

    QVector<QPoint> m_framePositionsOnMove;
private:
    QHash<QEvent::Type, int> m_received;
    QVector<QEvent::Type> m_order;
    QRegion m_exposeRegion;
};

void tst_QWindow::eventOrderOnShow()
{
    // Some platforms enforce minimum widths for windows, which can cause extra resize
    // events, so set the width to suitably large value to avoid those.
    QRect geometry(m_availableTopLeft + QPoint(80, 80), m_testWindowSize);

    Window window;
    window.setGeometry(geometry);
    window.show();
    QCoreApplication::processEvents();

    QTRY_COMPARE(window.received(QEvent::Show), 1);
    QTRY_COMPARE(window.received(QEvent::Resize), 1);
    QTRY_VERIFY(window.isExposed());

    QVERIFY(window.eventIndex(QEvent::Show) < window.eventIndex(QEvent::Resize));
    QVERIFY(window.eventIndex(QEvent::Resize) < window.eventIndex(QEvent::Expose));
}

void tst_QWindow::resizeEventAfterResize()
{
    // Some platforms enforce minimum widths for windows, which can cause extra resize
    // events, so set the width to suitably large value to avoid those.
    QRect geometry(m_availableTopLeft + QPoint(80, 80), m_testWindowSize * 2);

    Window window;
    window.setGeometry(geometry);
    window.showNormal();

    QTRY_COMPARE(window.received(QEvent::Resize), 1);

    // QTBUG-32706
    // Make sure we get a resizeEvent after calling resize
    window.resize(m_testWindowSize);

#if defined(Q_OS_BLACKBERRY) // "window" is the "root" window and will always be shown fullscreen
                              // so we only expect one resize event
    QTRY_COMPARE(window.received(QEvent::Resize), 1);
#else
    QTRY_COMPARE(window.received(QEvent::Resize), 2);
#endif
}

void tst_QWindow::positioning_data()
{
    QTest::addColumn<int>("windowflags");

    QTest::newRow("default") << int(Qt::Window | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::WindowFullscreenButtonHint);

#ifdef Q_OS_OSX
    QTest::newRow("fake") << int(Qt::Window | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
#endif
}

void tst_QWindow::positioning()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(
                QPlatformIntegration::NonFullScreenWindows)) {
        QSKIP("This platform does not support non-fullscreen windows");
    }

    if (qApp->platformName().toLower() == QLatin1String("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");

    // Some platforms enforce minimum widths for windows, which can cause extra resize
    // events, so set the width to suitably large value to avoid those.
    const QRect geometry(m_availableTopLeft + QPoint(80, 80), m_testWindowSize);

    QFETCH(int, windowflags);
    Window window((Qt::WindowFlags)windowflags);
    window.setGeometry(QRect(m_availableTopLeft + QPoint(20, 20), m_testWindowSize));
    window.setFramePosition(m_availableTopLeft + QPoint(40, 40)); // Move window around before show, size must not change.
    QCOMPARE(window.geometry().size(), m_testWindowSize);
    window.setGeometry(geometry);
    QCOMPARE(window.geometry(), geometry);
    //  explicitly use non-fullscreen show. show() can be fullscreen on some platforms
    window.showNormal();
    QCoreApplication::processEvents();

    QTest::qWaitForWindowExposed(&window);

    QMargins originalMargins = window.frameMargins();

    QCOMPARE(window.position(), window.framePosition() + QPoint(originalMargins.left(), originalMargins.top()));
    QVERIFY(window.frameGeometry().contains(window.geometry()));

    QPoint originalPos = window.position();
    QPoint originalFramePos = window.framePosition();

    window.reset();
    window.setWindowState(Qt::WindowFullScreen);
    QCoreApplication::processEvents();
    // On BB10 the window is the root window and fullscreen, so nothing is resized.
#if !defined(Q_OS_BLACKBERRY)
    QTRY_VERIFY(window.received(QEvent::Resize) > 0);
#endif

    QTest::qWait(2000);

    window.reset();
    window.setWindowState(Qt::WindowNoState);
    QCoreApplication::processEvents();
    // On BB10 the window is the root window and fullscreen, so nothing is resized.
#if !defined(Q_OS_BLACKBERRY)
    QTRY_VERIFY(window.received(QEvent::Resize) > 0);
#endif

    QTest::qWait(2000);

    QTRY_COMPARE(originalPos, window.position());
    QTRY_COMPARE(originalFramePos, window.framePosition());
    QTRY_COMPARE(originalMargins, window.frameMargins());

    // if our positioning is actually fully respected by the window manager
    // test whether it correctly handles frame positioning as well
    if (originalPos == geometry.topLeft() && (originalMargins.top() != 0 || originalMargins.left() != 0)) {
        QPoint framePos = QPlatformScreen::platformScreenForWindow(&window)->availableGeometry().center();

        window.reset();
        const QPoint oldFramePos = window.framePosition();
        window.setFramePosition(framePos);

        QTRY_VERIFY(window.received(QEvent::Move));
        if (window.framePosition() != framePos) {
            qDebug() << "About to fail auto-test. Here is some additional information:";
            qDebug() << "window.framePosition() == " << window.framePosition();
            qDebug() << "old frame position == " << oldFramePos;
            qDebug() << "We received " << window.received(QEvent::Move) << " move events";
            qDebug() << "frame positions after each move event:" << window.m_framePositionsOnMove;
        }
        QTRY_COMPARE(framePos, window.framePosition());
        QTRY_COMPARE(originalMargins, window.frameMargins());
        QCOMPARE(window.position(), window.framePosition() + QPoint(originalMargins.left(), originalMargins.top()));

        // and back to regular positioning

        window.reset();
        window.setPosition(originalPos);
        QTRY_VERIFY(window.received(QEvent::Move));
        QTRY_COMPARE(originalPos, window.position());
    }
}

void tst_QWindow::positioningDuringMinimized()
{
    // QTBUG-39544, setting a geometry in minimized state should work as well.
    if (QGuiApplication::platformName().compare("windows", Qt::CaseInsensitive))
        QSKIP("Not supported on this platform");
    Window window;
    window.setTitle(QStringLiteral("positioningDuringMinimized"));
    const QRect initialGeometry(m_availableTopLeft + QPoint(100, 100), m_testWindowSize);
    window.setGeometry(initialGeometry);
    window.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCOMPARE(window.geometry(), initialGeometry);
    window.setWindowState(Qt::WindowMinimized);
    QCOMPARE(window.geometry(), initialGeometry);
    const QRect newGeometry(initialGeometry.topLeft() + QPoint(50, 50), initialGeometry.size() + QSize(50, 50));
    window.setGeometry(newGeometry);
    QTRY_COMPARE(window.geometry(), newGeometry);
    window.setWindowState(Qt::WindowNoState);
    QTRY_COMPARE(window.geometry(), newGeometry);
}

void tst_QWindow::isExposed()
{
    QRect geometry(m_availableTopLeft + QPoint(80, 80), m_testWindowSize);

    Window window;
    window.setGeometry(geometry);
    QCOMPARE(window.geometry(), geometry);
    window.show();
    QCoreApplication::processEvents();

    QTRY_VERIFY(window.received(QEvent::Expose) > 0);
    QTRY_VERIFY(window.isExposed());

#ifndef Q_OS_WIN
    // This is a top-level window so assuming it is completely exposed, the
    // expose region must be (0, 0), (width, height). If this is not the case,
    // the platform plugin is sending expose events with a region in an
    // incorrect coordinate system.
    QRect r = window.exposeRegion().boundingRect();
    r = QRect(window.mapToGlobal(r.topLeft()), r.size());
    QCOMPARE(r, window.geometry());
#endif

    window.hide();

    if (qApp->platformName().toLower() == QLatin1String("wayland"))
        QSKIP("Wayland: This is flaky. Figure out why.");

    QCoreApplication::processEvents();
    QTRY_VERIFY(window.received(QEvent::Expose) > 1);
    QTRY_VERIFY(!window.isExposed());
}


void tst_QWindow::isActive()
{
    if (qApp->platformName().toLower() == QLatin1String("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");

    Window window;
    // Some platforms enforce minimum widths for windows, which can cause extra resize
    // events, so set the width to suitably large value to avoid those.
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QCoreApplication::processEvents();

    QTRY_VERIFY(window.isExposed());
#if defined(Q_OS_QNX) // We either need to create a eglSurface or a create a backing store
                      // and then post the window in order for screen to show the window
    QOpenGLContext context;
    context.create();
    context.makeCurrent(&window);
    context.swapBuffers(&window);
#endif
    QTRY_COMPARE(window.received(QEvent::Resize), 1);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &window);
    QVERIFY(window.isActive());

    Window child;
    child.setParent(&window);
    child.setGeometry(10, 10, 20, 20);
    child.show();

    QTRY_VERIFY(child.isExposed());

    child.requestActivate();

    QTRY_VERIFY(QGuiApplication::focusWindow() == &child);
    QVERIFY(child.isActive());

    // parent shouldn't receive new resize events from child being shown
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.received(QEvent::Resize), 1);
    QTRY_COMPARE(window.received(QEvent::FocusIn), 1);
    QTRY_COMPARE(window.received(QEvent::FocusOut), 1);
    QTRY_COMPARE(child.received(QEvent::FocusIn), 1);

    // child has focus
    QVERIFY(window.isActive());

    Window dialog;
    dialog.setTransientParent(&window);
    dialog.setGeometry(QRect(m_availableTopLeft + QPoint(110, 100), m_testWindowSize));
    dialog.show();

    dialog.requestActivate();

    QTRY_VERIFY(dialog.isExposed());
    QCoreApplication::processEvents();
    QTRY_COMPARE(dialog.received(QEvent::Resize), 1);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &dialog);
    QVERIFY(dialog.isActive());

    // transient child has focus
    QVERIFY(window.isActive());

    // parent is active
    QVERIFY(child.isActive());

    window.requestActivate();

    QTRY_VERIFY(QGuiApplication::focusWindow() == &window);
    QCoreApplication::processEvents();
    QTRY_COMPARE(dialog.received(QEvent::FocusOut), 1);
    QTRY_COMPARE(window.received(QEvent::FocusIn), 2);

    QVERIFY(window.isActive());

    // transient parent has focus
    QVERIFY(dialog.isActive());

    // parent has focus
    QVERIFY(child.isActive());
}

class InputTestWindow : public QWindow
{
public:
    void keyPressEvent(QKeyEvent *event) {
        keyPressCode = event->key();
    }
    void keyReleaseEvent(QKeyEvent *event) {
        keyReleaseCode = event->key();
    }
    void mousePressEvent(QMouseEvent *event) {
        if (ignoreMouse) {
            event->ignore();
        } else {
            ++mousePressedCount;
            mouseSequenceSignature += 'p';
            mousePressButton = event->button();
            mousePressScreenPos = event->screenPos();
            mousePressLocalPos = event->localPos();
            if (spinLoopWhenPressed)
                QCoreApplication::processEvents();
        }
    }
    void mouseReleaseEvent(QMouseEvent *event) {
        if (ignoreMouse) {
            event->ignore();
        } else {
            ++mouseReleasedCount;
            mouseSequenceSignature += 'r';
            mouseReleaseButton = event->button();
        }
    }
    void mouseMoveEvent(QMouseEvent *event) {
        if (ignoreMouse) {
            event->ignore();
        } else {
            ++mouseMovedCount;
            mouseMoveButton = event->button();
            mouseMoveScreenPos = event->screenPos();
        }
    }
    void mouseDoubleClickEvent(QMouseEvent *event) {
        if (ignoreMouse) {
            event->ignore();
        } else {
            ++mouseDoubleClickedCount;
            mouseSequenceSignature += 'd';
        }
    }
    void touchEvent(QTouchEvent *event) {
        if (ignoreTouch) {
            event->ignore();
            return;
        }
        touchEventType = event->type();
        QList<QTouchEvent::TouchPoint> points = event->touchPoints();
        for (int i = 0; i < points.count(); ++i) {
            switch (points.at(i).state()) {
            case Qt::TouchPointPressed:
                ++touchPressedCount;
                if (spinLoopWhenPressed)
                    QCoreApplication::processEvents();
                break;
            case Qt::TouchPointReleased:
                ++touchReleasedCount;
                break;
            case Qt::TouchPointMoved:
                ++touchMovedCount;
                break;
            default:
                break;
            }
        }
    }
    void resetCounters() {
        mousePressedCount = mouseReleasedCount = mouseMovedCount = mouseDoubleClickedCount = 0;
        mouseSequenceSignature = QString();
        touchPressedCount = touchReleasedCount = touchMovedCount = 0;
    }

    InputTestWindow() {
        keyPressCode = keyReleaseCode = 0;
        mousePressButton = mouseReleaseButton = mouseMoveButton = 0;
        ignoreMouse = ignoreTouch = false;
        spinLoopWhenPressed = false;
        resetCounters();
    }

    int keyPressCode, keyReleaseCode;
    int mousePressButton, mouseReleaseButton, mouseMoveButton;
    int mousePressedCount, mouseReleasedCount, mouseMovedCount, mouseDoubleClickedCount;
    QString mouseSequenceSignature;
    QPointF mousePressScreenPos, mouseMoveScreenPos, mousePressLocalPos;
    int touchPressedCount, touchReleasedCount, touchMovedCount;
    QEvent::Type touchEventType;

    bool ignoreMouse, ignoreTouch;

    bool spinLoopWhenPressed;
};

void tst_QWindow::testInputEvents()
{
    InputTestWindow window;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QWindowSystemInterface::handleKeyEvent(&window, QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
    QWindowSystemInterface::handleKeyEvent(&window, QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier);
    QCoreApplication::processEvents();
    QCOMPARE(window.keyPressCode, int(Qt::Key_A));
    QCOMPARE(window.keyReleaseCode, int(Qt::Key_A));

    QPointF local(12, 34);
    QWindowSystemInterface::handleMouseEvent(&window, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, local, local, Qt::NoButton);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressButton, int(Qt::LeftButton));
    QCOMPARE(window.mouseReleaseButton, int(Qt::LeftButton));
    QCOMPARE(window.mousePressLocalPos, local);

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1, tp2;
    tp1.id = 1;
    tp1.state = Qt::TouchPointPressed;
    tp1.area = QRect(10, 10, 4, 4);
    tp2.id = 2;
    tp2.state = Qt::TouchPointPressed;
    tp2.area = QRect(20, 20, 4, 4);
    points << tp1 << tp2;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = Qt::TouchPointReleased;
    points[1].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchPressedCount, 2);
    QTRY_COMPARE(window.touchReleasedCount, 2);

    // Now with null pointer as window. local param should not be utilized:
    // handleMouseEvent() with tlw == 0 means the event is in global coords only.
    window.mousePressButton = window.mouseReleaseButton = 0;
    QPointF nonWindowGlobal(window.geometry().topRight() + QPoint(200, 50)); // not inside the window
    QWindowSystemInterface::handleMouseEvent(0, nonWindowGlobal, nonWindowGlobal, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(0, nonWindowGlobal, nonWindowGlobal, Qt::NoButton);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressButton, 0);
    QCOMPARE(window.mouseReleaseButton, 0);
    QPointF windowGlobal = window.mapToGlobal(local.toPoint());
    QWindowSystemInterface::handleMouseEvent(0, windowGlobal, windowGlobal, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(0, windowGlobal, windowGlobal, Qt::NoButton);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressButton, int(Qt::LeftButton));
    QCOMPARE(window.mouseReleaseButton, int(Qt::LeftButton));
    QCOMPARE(window.mousePressScreenPos, windowGlobal);
    QCOMPARE(window.mousePressLocalPos, local); // the local we passed was bogus, verify that qGuiApp calculated the proper one
}

void tst_QWindow::touchToMouseTranslation()
{
    if (!QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::SynthesizeMouseFromTouchEvents).toBool())
        QSKIP("Mouse events are synthesized by the system on this platform.");
    InputTestWindow window;
    window.ignoreTouch = true;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1, tp2;
    const QRectF pressArea(101, 102, 4, 4);
    const QRectF moveArea(105, 108, 4, 4);
    tp1.id = 1;
    tp1.state = Qt::TouchPointPressed;
    tp1.area = pressArea;
    tp2.id = 2;
    tp2.state = Qt::TouchPointPressed;
    points << tp1 << tp2;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    // Now an update but with changed list order. The mouse event should still
    // be generated from the point with id 1.
    tp1.id = 2;
    tp1.state = Qt::TouchPointStationary;
    tp2.id = 1;
    tp2.state = Qt::TouchPointMoved;
    tp2.area = moveArea;
    points.clear();
    points << tp1 << tp2;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = Qt::TouchPointReleased;
    points[1].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();

    QTRY_COMPARE(window.mousePressButton, int(Qt::LeftButton));
    QTRY_COMPARE(window.mouseReleaseButton, int(Qt::LeftButton));
    QTRY_COMPARE(window.mousePressScreenPos, pressArea.center());
    QTRY_COMPARE(window.mouseMoveScreenPos, moveArea.center());

    window.mousePressButton = 0;
    window.mouseReleaseButton = 0;

    window.ignoreTouch = false;

    points[0].state = Qt::TouchPointPressed;
    points[1].state = Qt::TouchPointPressed;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = Qt::TouchPointReleased;
    points[1].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();

    // no new mouse events should be generated since the input window handles the touch events
    QTRY_COMPARE(window.mousePressButton, 0);
    QTRY_COMPARE(window.mouseReleaseButton, 0);

    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);

    window.ignoreTouch = true;
    points[0].state = Qt::TouchPointPressed;
    points[1].state = Qt::TouchPointPressed;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = Qt::TouchPointReleased;
    points[1].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();

    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);

    // mouse event synthesizing disabled
    QTRY_COMPARE(window.mousePressButton, 0);
    QTRY_COMPARE(window.mouseReleaseButton, 0);
}

void tst_QWindow::mouseToTouchTranslation()
{
    qApp->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, true);

    InputTestWindow window;
    window.ignoreMouse = true;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::NoButton);
    QCoreApplication::processEvents();

    qApp->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);

    QTRY_COMPARE(window.touchPressedCount, 1);
    QTRY_COMPARE(window.touchReleasedCount, 1);

    qApp->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, true);

    window.ignoreMouse = false;

    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::NoButton);
    QCoreApplication::processEvents();

    qApp->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);

    // no new touch events should be generated since the input window handles the mouse events
    QTRY_COMPARE(window.touchPressedCount, 1);
    QTRY_COMPARE(window.touchReleasedCount, 1);

    window.ignoreMouse = true;

    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::NoButton);
    QCoreApplication::processEvents();

    // touch event synthesis disabled
    QTRY_COMPARE(window.touchPressedCount, 1);
    QTRY_COMPARE(window.touchReleasedCount, 1);


}

void tst_QWindow::mouseToTouchLoop()
{
    // make sure there's no infinite loop when synthesizing both ways
    qApp->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, true);
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);

    InputTestWindow window;
    window.ignoreMouse = true;
    window.ignoreTouch = true;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, QPoint(10, 10), window.mapToGlobal(QPoint(10, 10)), Qt::NoButton);
    QCoreApplication::processEvents();

    qApp->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);
}

void tst_QWindow::touchCancel()
{
    InputTestWindow window;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1;
    tp1.id = 1;

    // Start a touch.
    tp1.state = Qt::TouchPointPressed;
    tp1.area = QRect(10, 10, 4, 4);
    points << tp1;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchEventType, QEvent::TouchBegin);
    QTRY_COMPARE(window.touchPressedCount, 1);

    // Cancel the active touch sequence.
    QWindowSystemInterface::handleTouchCancelEvent(&window, touchDevice);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchEventType, QEvent::TouchCancel);

    // Send a move -> will not be delivered due to the cancellation.
    QTRY_COMPARE(window.touchMovedCount, 0);
    points[0].state = Qt::TouchPointMoved;
    tp1.area.adjust(2, 2, 2, 2);
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchMovedCount, 0);

    // Likewise. The only allowed transition is TouchCancel -> TouchBegin.
    QTRY_COMPARE(window.touchReleasedCount, 0);
    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchReleasedCount, 0);

    // Start a new sequence -> from this point on everything should go through normally.
    points[0].state = Qt::TouchPointPressed;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchEventType, QEvent::TouchBegin);
    QTRY_COMPARE(window.touchPressedCount, 2);

    points[0].state = Qt::TouchPointMoved;
    tp1.area.adjust(2, 2, 2, 2);
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchMovedCount, 1);

    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchReleasedCount, 1);
}

void tst_QWindow::touchCancelWithTouchToMouse()
{
    if (!QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::SynthesizeMouseFromTouchEvents).toBool())
        QSKIP("Mouse events are synthesized by the system on this platform.");
    InputTestWindow window;
    window.ignoreTouch = true;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1;
    tp1.id = 1;

    tp1.state = Qt::TouchPointPressed;
    tp1.area = QRect(100, 100, 4, 4);
    points << tp1;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.mousePressButton, int(Qt::LeftButton));
    QTRY_COMPARE(window.mousePressScreenPos, points[0].area.center());

    // Cancel the touch. Should result in a mouse release for windows that have
    // have an active touch-to-mouse sequence.
    QWindowSystemInterface::handleTouchCancelEvent(0, touchDevice);
    QCoreApplication::processEvents();

    QTRY_COMPARE(window.mouseReleaseButton, int(Qt::LeftButton));

    // Now change the window to accept touches.
    window.mousePressButton = window.mouseReleaseButton = 0;
    window.ignoreTouch = false;

    // Send a touch, there will be no mouse event generated.
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.mousePressButton, 0);

    // Cancel the touch. It should not result in a mouse release with this window.
    QWindowSystemInterface::handleTouchCancelEvent(0, touchDevice);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.mouseReleaseButton, 0);
}

void tst_QWindow::touchInterruptedByPopup()
{
    InputTestWindow window;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1;
    tp1.id = 1;

    // Start a touch.
    tp1.state = Qt::TouchPointPressed;
    tp1.area = QRect(10, 10, 4, 4);
    points << tp1;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchEventType, QEvent::TouchBegin);
    QTRY_COMPARE(window.touchPressedCount, 1);

    // Launch a popup window
    InputTestWindow popup;
    popup.setFlags(Qt::Popup);
    popup.setModality(Qt::WindowModal);
    popup.resize(m_testWindowSize /  2);
    popup.setTransientParent(&window);
    popup.show();
    QVERIFY(QTest::qWaitForWindowExposed(&popup));

    // Send a move -> will not be delivered to the original window
    // (TODO verify where it is forwarded, after we've defined that)
    QTRY_COMPARE(window.touchMovedCount, 0);
    points[0].state = Qt::TouchPointMoved;
    tp1.area.adjust(2, 2, 2, 2);
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchMovedCount, 0);

    // Send a touch end -> will not be delivered to the original window
    QTRY_COMPARE(window.touchReleasedCount, 0);
    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchReleasedCount, 0);

    // Due to temporary fix for QTBUG-37371: the original window should receive a TouchCancel
    QTRY_COMPARE(window.touchEventType, QEvent::TouchCancel);
}

void tst_QWindow::orientation()
{
    qRegisterMetaType<Qt::ScreenOrientation>("Qt::ScreenOrientation");

    QWindow window;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.create();

    window.reportContentOrientationChange(Qt::PortraitOrientation);
    QCOMPARE(window.contentOrientation(), Qt::PortraitOrientation);

    window.reportContentOrientationChange(Qt::PrimaryOrientation);
    QCOMPARE(window.contentOrientation(), Qt::PrimaryOrientation);

    QSignalSpy spy(&window, SIGNAL(contentOrientationChanged(Qt::ScreenOrientation)));
    window.reportContentOrientationChange(Qt::LandscapeOrientation);
    QCOMPARE(spy.count(), 1);
}

void tst_QWindow::sizes()
{
    QWindow window;

    QSignalSpy minimumWidthSpy(&window, SIGNAL(minimumWidthChanged(int)));
    QSignalSpy minimumHeightSpy(&window, SIGNAL(minimumHeightChanged(int)));
    QSignalSpy maximumWidthSpy(&window, SIGNAL(maximumWidthChanged(int)));
    QSignalSpy maximumHeightSpy(&window, SIGNAL(maximumHeightChanged(int)));

    QSize oldMaximum = window.maximumSize();

    window.setMinimumWidth(10);
    QCOMPARE(window.minimumWidth(), 10);
    QCOMPARE(window.minimumHeight(), 0);
    QCOMPARE(window.minimumSize(), QSize(10, 0));
    QCOMPARE(window.maximumSize(), oldMaximum);
    QCOMPARE(minimumWidthSpy.count(), 1);
    QCOMPARE(minimumHeightSpy.count(), 0);
    QCOMPARE(maximumWidthSpy.count(), 0);
    QCOMPARE(maximumHeightSpy.count(), 0);

    window.setMinimumHeight(10);
    QCOMPARE(window.minimumWidth(), 10);
    QCOMPARE(window.minimumHeight(), 10);
    QCOMPARE(window.minimumSize(), QSize(10, 10));
    QCOMPARE(window.maximumSize(), oldMaximum);
    QCOMPARE(minimumWidthSpy.count(), 1);
    QCOMPARE(minimumHeightSpy.count(), 1);
    QCOMPARE(maximumWidthSpy.count(), 0);
    QCOMPARE(maximumHeightSpy.count(), 0);

    window.setMaximumWidth(100);
    QCOMPARE(window.maximumWidth(), 100);
    QCOMPARE(window.maximumHeight(), oldMaximum.height());
    QCOMPARE(window.minimumSize(), QSize(10, 10));
    QCOMPARE(window.maximumSize(), QSize(100, oldMaximum.height()));
    QCOMPARE(minimumWidthSpy.count(), 1);
    QCOMPARE(minimumHeightSpy.count(), 1);
    QCOMPARE(maximumWidthSpy.count(), 1);
    QCOMPARE(maximumHeightSpy.count(), 0);

    window.setMaximumHeight(100);
    QCOMPARE(window.maximumWidth(), 100);
    QCOMPARE(window.maximumHeight(), 100);
    QCOMPARE(window.minimumSize(), QSize(10, 10));
    QCOMPARE(window.maximumSize(), QSize(100, 100));
    QCOMPARE(minimumWidthSpy.count(), 1);
    QCOMPARE(minimumHeightSpy.count(), 1);
    QCOMPARE(maximumWidthSpy.count(), 1);
    QCOMPARE(maximumHeightSpy.count(), 1);
}

void tst_QWindow::close()
{
    QWindow a;
    QWindow b;
    QWindow c(&a);

    a.show();
    b.show();

    // we can not close a non top level window
    QVERIFY(!c.close());
    QVERIFY(a.close());
    QVERIFY(b.close());
}

void tst_QWindow::activateAndClose()
{
    if (qApp->platformName().toLower() == QLatin1String("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");

    for (int i = 0; i < 10; ++i)  {
       QWindow window;
#if defined(Q_OS_QNX)
       window.setSurfaceType(QSurface::OpenGLSurface);
#endif
       // qWaitForWindowActive will block for the duration of
       // of the timeout if the window is at 0,0
       window.setGeometry(QGuiApplication::primaryScreen()->availableGeometry().adjusted(1, 1, -1, -1));
       window.showNormal();
#if defined(Q_OS_QNX) // We either need to create a eglSurface or a create a backing store
                      // and then post the window in order for screen to show the window
       QTest::qWaitForWindowExposed(&window);
       QOpenGLContext context;
       context.create();
       context.makeCurrent(&window);
       context.swapBuffers(&window);
#endif
       window.requestActivate();
       QVERIFY(QTest::qWaitForWindowActive(&window));
       QCOMPARE(qGuiApp->focusWindow(), &window);
    }
}

void tst_QWindow::mouseEventSequence()
{
    int doubleClickInterval = qGuiApp->styleHints()->mouseDoubleClickInterval();

    InputTestWindow window;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    ulong timestamp = 0;
    QPointF local(12, 34);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 1);
    QCOMPARE(window.mouseReleasedCount, 1);
    QCOMPARE(window.mouseDoubleClickedCount, 0);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("pr"));

    window.resetCounters();
    timestamp += doubleClickInterval;

    // A double click must result in press, release, press, doubleclick, release.
    // Check that no unexpected event suppression occurs and that the order is correct.
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 2);
    QCOMPARE(window.mouseReleasedCount, 2);
    QCOMPARE(window.mouseDoubleClickedCount, 1);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("prpdr"));

    timestamp += doubleClickInterval;
    window.resetCounters();

    // Triple click = double + single click
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 3);
    QCOMPARE(window.mouseReleasedCount, 3);
    QCOMPARE(window.mouseDoubleClickedCount, 1);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("prpdrpr"));

    timestamp += doubleClickInterval;
    window.resetCounters();

    // Two double clicks.
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 4);
    QCOMPARE(window.mouseReleasedCount, 4);
    QCOMPARE(window.mouseDoubleClickedCount, 2);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("prpdrprpdr"));

    timestamp += doubleClickInterval;
    window.resetCounters();

    // Four clicks, none of which qualifies as a double click.
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    timestamp += doubleClickInterval;
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    timestamp += doubleClickInterval;
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    timestamp += doubleClickInterval;
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, timestamp++, local, local, Qt::NoButton);
    timestamp += doubleClickInterval;
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 4);
    QCOMPARE(window.mouseReleasedCount, 4);
    QCOMPARE(window.mouseDoubleClickedCount, 0);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("prprprpr"));
}

void tst_QWindow::windowModality()
{
    qRegisterMetaType<Qt::WindowModality>("Qt::WindowModality");

    QWindow window;
    QSignalSpy spy(&window, SIGNAL(modalityChanged(Qt::WindowModality)));

    QCOMPARE(window.modality(), Qt::NonModal);
    window.setModality(Qt::NonModal);
    QCOMPARE(window.modality(), Qt::NonModal);
    QCOMPARE(spy.count(), 0);

    window.setModality(Qt::WindowModal);
    QCOMPARE(window.modality(), Qt::WindowModal);
    QCOMPARE(spy.count(), 1);
    window.setModality(Qt::WindowModal);
    QCOMPARE(window.modality(), Qt::WindowModal);
    QCOMPARE(spy.count(), 1);

    window.setModality(Qt::ApplicationModal);
    QCOMPARE(window.modality(), Qt::ApplicationModal);
    QCOMPARE(spy.count(), 2);
    window.setModality(Qt::ApplicationModal);
    QCOMPARE(window.modality(), Qt::ApplicationModal);
    QCOMPARE(spy.count(), 2);

    window.setModality(Qt::NonModal);
    QCOMPARE(window.modality(), Qt::NonModal);
    QCOMPARE(spy.count(), 3);
}

void tst_QWindow::inputReentrancy()
{
    InputTestWindow window;
    window.spinLoopWhenPressed = true;

    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Queue three events.
    QPointF local(12, 34);
    QWindowSystemInterface::handleMouseEvent(&window, local, local, Qt::LeftButton);
    local += QPointF(2, 2);
    QWindowSystemInterface::handleMouseEvent(&window, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, local, local, Qt::NoButton);
    // Process them. However, the event handler for the press will also call
    // processEvents() so the move and release will be delivered before returning
    // from mousePressEvent(). The point is that no events should get lost.
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressButton, int(Qt::LeftButton));
    QCOMPARE(window.mouseReleaseButton, int(Qt::LeftButton));
    QCOMPARE(window.mousePressedCount, 1);
    QCOMPARE(window.mouseMovedCount, 1);
    QCOMPARE(window.mouseReleasedCount, 1);

    // Now the same for touch.
    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1;
    tp1.id = 1;
    tp1.state = Qt::TouchPointPressed;
    tp1.area = QRectF(10, 10, 4, 4);
    points << tp1;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = Qt::TouchPointMoved;
    points[0].area = QRectF(20, 20, 8, 8);
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QCOMPARE(window.touchPressedCount, 1);
    QCOMPARE(window.touchMovedCount, 1);
    QCOMPARE(window.touchReleasedCount, 1);
}

#ifndef QT_NO_TABLETEVENT
class TabletTestWindow : public QWindow
{
public:
    TabletTestWindow() : eventType(0) { }
    void tabletEvent(QTabletEvent *ev) {
        eventType = ev->type();
        eventGlobal = ev->globalPosF();
        eventLocal = ev->posF();
        eventDevice = ev->device();
    }
    int eventType;
    QPointF eventGlobal, eventLocal;
    int eventDevice;
    bool eventFilter(QObject *obj, QEvent *ev) {
        if (ev->type() == QEvent::TabletEnterProximity
                || ev->type() == QEvent::TabletLeaveProximity) {
            eventType = ev->type();
            QTabletEvent *te = static_cast<QTabletEvent *>(ev);
            eventDevice = te->device();
        }
        return QWindow::eventFilter(obj, ev);
    }
};
#endif

void tst_QWindow::tabletEvents()
{
#ifndef QT_NO_TABLETEVENT
    TabletTestWindow window;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(10, 10), m_testWindowSize));
    qGuiApp->installEventFilter(&window);

    QPoint local(10, 10);
    QPoint global = window.mapToGlobal(local);
    QWindowSystemInterface::handleTabletEvent(&window, true, local, global, 1, 2, 0.5, 1, 2, 0.1, 0, 0, 0);
    QCoreApplication::processEvents();
    QTRY_VERIFY(window.eventType == QEvent::TabletPress);
    QTRY_COMPARE(window.eventGlobal.toPoint(), global);
    QTRY_COMPARE(window.eventLocal.toPoint(), local);
    QWindowSystemInterface::handleTabletEvent(&window, false, local, global, 1, 2, 0.5, 1, 2, 0.1, 0, 0, 0);
    QCoreApplication::processEvents();
    QTRY_VERIFY(window.eventType == QEvent::TabletRelease);

    QWindowSystemInterface::handleTabletEnterProximityEvent(1, 2, 3);
    QCoreApplication::processEvents();
    QTRY_VERIFY(window.eventType == QEvent::TabletEnterProximity);
    QTRY_COMPARE(window.eventDevice, 1);

    QWindowSystemInterface::handleTabletLeaveProximityEvent(1, 2, 3);
    QCoreApplication::processEvents();
    QTRY_VERIFY(window.eventType == QEvent::TabletLeaveProximity);
    QTRY_COMPARE(window.eventDevice, 1);

#endif
}

void tst_QWindow::windowModality_QTBUG27039()
{
    if (qApp->platformName().toLower() == QLatin1String("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");

    QWindow parent;
    parent.setGeometry(QRect(m_availableTopLeft + QPoint(10, 10), m_testWindowSize));
    parent.show();

    InputTestWindow modalA;
    modalA.setTransientParent(&parent);
    modalA.setGeometry(QRect(m_availableTopLeft + QPoint(20, 10), m_testWindowSize));
    modalA.setModality(Qt::ApplicationModal);
    modalA.show();

    InputTestWindow modalB;
    modalB.setTransientParent(&parent);
    modalA.setGeometry(QRect(m_availableTopLeft + QPoint(30, 10), m_testWindowSize));
    modalB.setModality(Qt::ApplicationModal);
    modalB.show();

    QPointF local(5, 5);
    QWindowSystemInterface::handleMouseEvent(&modalA, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&modalA, local, local, Qt::NoButton);
    QWindowSystemInterface::handleMouseEvent(&modalB, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&modalB, local, local, Qt::NoButton);
    QCoreApplication::processEvents();

    // modal A should be blocked since it was shown first, but modal B should not be blocked
    QCOMPARE(modalB.mousePressedCount, 1);
    QCOMPARE(modalA.mousePressedCount, 0);

    modalB.hide();
    QWindowSystemInterface::handleMouseEvent(&modalA, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&modalA, local, local, Qt::NoButton);
    QCoreApplication::processEvents();

    // modal B has been hidden, modal A should be unblocked again
    QCOMPARE(modalA.mousePressedCount, 1);
}

void tst_QWindow::visibility()
{
    qRegisterMetaType<Qt::WindowModality>("QWindow::Visibility");

    QWindow window;
    QSignalSpy spy(&window, SIGNAL(visibilityChanged(QWindow::Visibility)));

    window.setVisibility(QWindow::AutomaticVisibility);
    QVERIFY(window.isVisible());
    QVERIFY(window.visibility() != QWindow::Hidden);
    QVERIFY(window.visibility() != QWindow::AutomaticVisibility);
    QCOMPARE(spy.count(), 1);
    spy.clear();

    window.setVisibility(QWindow::Hidden);
    QVERIFY(!window.isVisible());
    QCOMPARE(window.visibility(), QWindow::Hidden);
    QCOMPARE(spy.count(), 1);
    spy.clear();

    window.setVisibility(QWindow::FullScreen);
    QVERIFY(window.isVisible());
    QCOMPARE(window.windowState(), Qt::WindowFullScreen);
    QCOMPARE(window.visibility(), QWindow::FullScreen);
    QCOMPARE(spy.count(), 1);
    spy.clear();

    window.setWindowState(Qt::WindowNoState);
    QCOMPARE(window.visibility(), QWindow::Windowed);
    QCOMPARE(spy.count(), 1);
    spy.clear();

    window.setVisible(false);
    QCOMPARE(window.visibility(), QWindow::Hidden);
    QCOMPARE(spy.count(), 1);
    spy.clear();
}

void tst_QWindow::mask()
{
    QRegion mask = QRect(10, 10, 800 - 20, 600 - 20);

    QWindow window;
    window.resize(800, 600);
    window.setMask(mask);

    QCOMPARE(window.mask(), QRegion());

    window.create();
    window.setMask(mask);

    QCOMPARE(window.mask(), mask);
}

void tst_QWindow::initialSize()
{
    if (qApp->platformName().toLower() == QLatin1String("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");

    QSize defaultSize(0,0);
    {
    Window w;
    w.show();
    QTRY_VERIFY(w.width() > 0);
    QTRY_VERIFY(w.height() > 0);
    defaultSize = QSize(w.width(), w.height());
    }
    {
    Window w;
    w.setWidth(m_testWindowSize.width());
    w.show();
#if defined(Q_OS_BLACKBERRY) // "window" is the "root" window and will always be shown fullscreen
                              // so we only expect one resize event
    QTRY_COMPARE(w.width(), qGuiApp->primaryScreen()->availableGeometry().width());
#else
    QTRY_COMPARE(w.width(), m_testWindowSize.width());
#endif
    QTRY_VERIFY(w.height() > 0);
    }
    {
    Window w;
    const QSize testSize(m_testWindowSize.width(), 42);
    w.resize(testSize);
    w.show();

#if defined(Q_OS_BLACKBERRY) // "window" is the "root" window and will always be shown fullscreen
                              // so we only expect one resize event
    const QSize expectedSize = QGuiApplication::primaryScreen()->availableGeometry().size();
#else
    const QSize expectedSize = testSize;
#endif
    QTRY_COMPARE(w.size(), expectedSize);
    }
}

void tst_QWindow::modalDialog()
{
    if (qApp->platformName().toLower() == QLatin1String("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");

    QWindow normalWindow;
    normalWindow.setFramePosition(m_availableTopLeft + QPoint(80, 80));
    normalWindow.resize(m_testWindowSize);
    normalWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&normalWindow));

    QWindow dialog;
    dialog.setFramePosition(m_availableTopLeft + QPoint(200, 200));
    dialog.resize(m_testWindowSize);
    dialog.setModality(Qt::ApplicationModal);
    dialog.setFlags(Qt::Dialog);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    normalWindow.requestActivate();

    QGuiApplication::sync();
    QGuiApplication::processEvents();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &dialog);
}

void tst_QWindow::modalDialogClosingOneOfTwoModal()
{
    if (qApp->platformName().toLower() == QLatin1String("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");

    QWindow normalWindow;
    normalWindow.setFramePosition(m_availableTopLeft + QPoint(80, 80));
    normalWindow.resize(m_testWindowSize);
    normalWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&normalWindow));

    QWindow first_dialog;
    first_dialog.setFramePosition(m_availableTopLeft + QPoint(200, 200));
    first_dialog.resize(m_testWindowSize);
    first_dialog.setModality(Qt::ApplicationModal);
    first_dialog.setFlags(Qt::Dialog);
    first_dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&first_dialog));

    {
        QWindow second_dialog;
        second_dialog.setFramePosition(m_availableTopLeft + QPoint(300, 300));
        second_dialog.resize(m_testWindowSize);
        second_dialog.setModality(Qt::ApplicationModal);
        second_dialog.setFlags(Qt::Dialog);
        second_dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&second_dialog));

        QTRY_COMPARE(QGuiApplication::focusWindow(), &second_dialog);

        second_dialog.close();
    }

    QGuiApplication::sync();
    QGuiApplication::processEvents();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &first_dialog);
}

void tst_QWindow::modalWithChildWindow()
{
    if (qApp->platformName().toLower() == QLatin1String("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");

    QWindow normalWindow;
    normalWindow.setFramePosition(m_availableTopLeft + QPoint(80, 80));
    normalWindow.resize(m_testWindowSize);
    normalWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&normalWindow));

    QWindow tlw_dialog;
    tlw_dialog.setFramePosition(m_availableTopLeft + QPoint(200, 200));
    tlw_dialog.resize(m_testWindowSize);
    tlw_dialog.setModality(Qt::ApplicationModal);
    tlw_dialog.setFlags(Qt::Dialog);
    tlw_dialog.create();

    QWindow sub_window(&tlw_dialog);
    sub_window.resize(200,300);
    sub_window.show();

    tlw_dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tlw_dialog));
    QVERIFY(QTest::qWaitForWindowExposed(&sub_window));

    QTRY_COMPARE(QGuiApplication::focusWindow(), &tlw_dialog);

    sub_window.requestActivate();
    QGuiApplication::sync();
    QGuiApplication::processEvents();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &sub_window);
}

void tst_QWindow::modalWindowModallity()
{
    if (qApp->platformName().toLower() == QLatin1String("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");

    QWindow normal_window;
    normal_window.setFramePosition(m_availableTopLeft + QPoint(80, 80));
    normal_window.resize(m_testWindowSize);
    normal_window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&normal_window));

    QWindow parent_to_modal;
    parent_to_modal.setFramePosition(normal_window.geometry().topRight() + QPoint(100, 0));
    parent_to_modal.resize(m_testWindowSize);
    parent_to_modal.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent_to_modal));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &parent_to_modal);

    QWindow modal_dialog;
    modal_dialog.resize(m_testWindowSize);
    modal_dialog.setFramePosition(normal_window.geometry().bottomLeft() + QPoint(0, 100));
    modal_dialog.setModality(Qt::WindowModal);
    modal_dialog.setFlags(Qt::Dialog);
    modal_dialog.setTransientParent(&parent_to_modal);
    modal_dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&modal_dialog));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &modal_dialog);

    normal_window.requestActivate();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &normal_window);

}

void tst_QWindow::modalWindowPosition()
{
    QWindow window;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(100, 100), m_testWindowSize));
    // Allow for any potential resizing due to constraints
    QRect origGeo = window.geometry();
    window.setModality(Qt::WindowModal);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCOMPARE(window.geometry(), origGeo);
}

class ColoredWindow : public QRasterWindow {
public:
    explicit ColoredWindow(const QColor &color, QWindow *parent = 0) : QRasterWindow(parent), m_color(color) {}
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE
    {
        QPainter p(this);
        p.fillRect(QRect(QPoint(0, 0), size()), m_color);
    }

private:
    const QColor m_color;
};

static bool isNativeWindowVisible(const QWindow *window)
{
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    return IsWindowVisible(reinterpret_cast<HWND>(window->winId()));
#else
    Q_UNIMPLEMENTED();
    return window->isVisible();
#endif
}

void tst_QWindow::windowsTransientChildren()
{
    if (QGuiApplication::platformName().compare(QStringLiteral("windows"), Qt::CaseInsensitive))
        QSKIP("Windows only test");

    ColoredWindow mainWindow(Qt::yellow);
    mainWindow.setGeometry(QRect(m_availableTopLeft + QPoint(100, 100), m_testWindowSize));
    mainWindow.setTitle(QStringLiteral("Main"));
    ColoredWindow child(Qt::blue, &mainWindow);
    child.setGeometry(QRect(QPoint(0, 0), m_testWindowSize / 2));

    ColoredWindow dialog(Qt::red);
    dialog.setGeometry(QRect(m_availableTopLeft + QPoint(200, 200), m_testWindowSize));
    dialog.setTitle(QStringLiteral("Dialog"));
    dialog.setTransientParent(&mainWindow);

    mainWindow.show();
    child.show();
    dialog.show();

    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    mainWindow.setWindowState(Qt::WindowMinimized);
    QVERIFY(!isNativeWindowVisible(&dialog));
    dialog.hide();
    mainWindow.setWindowState(Qt::WindowNoState);
    // QTBUG-40696, transient children hidden by Qt should not be re-shown by Windows.
    QVERIFY(!isNativeWindowVisible(&dialog));
    QVERIFY(isNativeWindowVisible(&child)); // Real children should be visible.
}

#include <tst_qwindow.moc>
QTEST_MAIN(tst_QWindow)

