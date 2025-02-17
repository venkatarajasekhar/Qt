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

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>

#include "../../shared/util.h"

class tst_QQuickItemLayer: public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickItemLayer();

    QImage runTest(const QString &fileName)
    {
#if defined(Q_OS_BLACKBERRY)
        QWindow dummy;            // On BlackBerry first window is always full screen,
        dummy.showFullScreen();   // so make test window a second window.
#endif
        QQuickView view;
        view.setSource(testFileUrl(fileName));

        view.showNormal();
        QTest::qWaitForWindowExposed(&view);

        return view.grabWindow();
    }

private slots:
    void layerEnabled();
    void layerSmooth();
    void layerMipmap();
    void layerEffect();

    void layerVisibility_data();
    void layerVisibility();

    void layerSourceRect();

    void layerZOrder_data();
    void layerZOrder();

    void layerIsTextureProvider();

    void changeZOrder_data();
    void changeZOrder();

    void toggleLayerAndEffect();
    void disableLayer();
    void changeSamplerName();
    void itemEffect();
    void rectangleEffect();

private:
    bool m_isMesaSoftwareRasterizer;
    int m_mesaVersion;
};

tst_QQuickItemLayer::tst_QQuickItemLayer()
    : m_mesaVersion(0)
{
    QWindow window;
    QOpenGLContext context;
    window.setSurfaceType(QWindow::OpenGLSurface);
    window.create();
    context.create();
    context.makeCurrent(&window);
    const char *vendor = (const char *)context.functions()->glGetString(GL_VENDOR);
    const char *renderer = (const char *)context.functions()->glGetString(GL_RENDERER);
    m_isMesaSoftwareRasterizer = strcmp(vendor, "Mesa Project") == 0
            && strcmp(renderer, "Software Rasterizer") == 0;
    if (m_isMesaSoftwareRasterizer) {
        // Expects format: <OpenGL version> Mesa <Mesa version>[-devel] [...]
        const char *version = (const char *)context.functions()->glGetString(GL_VERSION);
        QList<QByteArray> list = QByteArray(version).split(' ');
        if (list.size() >= 3) {
            list = list.at(2).split('-').at(0).split('.');
            int major = 0;
            int minor = 0;
            int patch = 0;
            if (list.size() >= 1)
                major = list.at(0).toInt();
            if (list.size() >= 2)
                minor = list.at(1).toInt();
            if (list.size() >= 3)
                patch = list.at(2).toInt();
            m_mesaVersion = QT_VERSION_CHECK(major, minor, patch);
        }
    }
}

// The test draws a red and a blue box next to each other and tests that the
// output is still red and blue on the left and right and a combination of
// the two in the middle.

void tst_QQuickItemLayer::layerSmooth()
{
    if (m_isMesaSoftwareRasterizer && m_mesaVersion < QT_VERSION_CHECK(7, 11, 0))
        QSKIP("Mesa Software Rasterizer below version 7.11 does not render this test correctly.");
    QImage fb = runTest("Smooth.qml");
    QCOMPARE(fb.pixel(0, 0), qRgb(0xff, 0, 0));
    QCOMPARE(fb.pixel(fb.width() - 1, 0), qRgb(0, 0, 0xff));

    uint pixel = fb.pixel(fb.width() / 2, 0);
    QVERIFY(qRed(pixel) > 0);
    QVERIFY(qBlue(pixel) > 0);
}



// The test draws a gradient at a small size into a layer and scales the
// layer. If the layer is enabled there should be very visible bands in
// the gradient.

void tst_QQuickItemLayer::layerEnabled()
{
    if (m_isMesaSoftwareRasterizer && m_mesaVersion < QT_VERSION_CHECK(7, 11, 0))
        QSKIP("Mesa Software Rasterizer below version 7.11 does not render this test correctly.");
    QImage fb = runTest("Enabled.qml");
    // Verify the banding
    QCOMPARE(fb.pixel(0, 0), fb.pixel(0, 1));
    // Verify the gradient
    QVERIFY(fb.pixel(0, 0) != fb.pixel(0, fb.height() - 1));
}



// The test draws a one pixel wide line and scales it down by more than a a factor 2
// If mipmpping works, the pixels should be gray, not white or black

void tst_QQuickItemLayer::layerMipmap()
{
    if (m_isMesaSoftwareRasterizer)
        QSKIP("Mipmapping does not work with the Mesa Software Rasterizer.");
    QImage fb = runTest("Mipmap.qml");
    QVERIFY(fb.pixel(0, 0) != 0xff000000);
    QVERIFY(fb.pixel(0, 0) != 0xffffffff);
}



// The test implements an rgb swapping effect sourced from a blue rectangle. The
// resulting pixel should be red

void tst_QQuickItemLayer::layerEffect()
{
    if (m_isMesaSoftwareRasterizer && m_mesaVersion < QT_VERSION_CHECK(7, 11, 0))
        QSKIP("Mesa Software Rasterizer below version 7.11 does not render this test correctly.");
    QImage fb = runTest("Effect.qml");
    QCOMPARE(fb.pixel(0, 0), qRgb(0xff, 0, 0));
    QCOMPARE(fb.pixel(fb.width() - 1, 0), qRgb(0, 0xff, 0));
}



// The test draws a rectangle and verifies that there is padding on each side
// as the source rect spans outside the item. The padding is verified using
// a shader that pads transparent to blue. Everything else is red.
void tst_QQuickItemLayer::layerSourceRect()
{
    if (m_isMesaSoftwareRasterizer && m_mesaVersion < QT_VERSION_CHECK(7, 11, 0))
        QSKIP("Mesa Software Rasterizer below version 7.11 does not render this test correctly.");

    QImage fb = runTest("SourceRect.qml");

    // Check that the edges are converted to blue
    QCOMPARE(fb.pixel(0, 0), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(fb.width() - 1, 0), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(0, fb.height() - 1), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(fb.width() - 1, fb.height() - 1), qRgb(0, 0, 0xff));

    // The center pixel should be red
    QCOMPARE(fb.pixel(fb.width() / 2, fb.height() / 2), qRgb(0xff, 0, 0));
}



// Same as the effect test up above, but this time use the item
// directly in a stand alone ShaderEffect
void tst_QQuickItemLayer::layerIsTextureProvider()
{
    if (m_isMesaSoftwareRasterizer && m_mesaVersion < QT_VERSION_CHECK(7, 11, 0))
        QSKIP("Mesa Software Rasterizer below version 7.11 does not render this test correctly.");
    QImage fb = runTest("TextureProvider.qml");
    QCOMPARE(fb.pixel(0, 0), qRgb(0xff, 0, 0));
    QCOMPARE(fb.pixel(fb.width() - 1, 0), qRgb(0, 0xff, 0));
}


void tst_QQuickItemLayer::layerVisibility_data()
{
    QTest::addColumn<bool>("visible");
    QTest::addColumn<bool>("effect");
    QTest::addColumn<qreal>("opacity");

    QTest::newRow("!effect, !visible, a=1") << false << false << 1.;
    QTest::newRow("!effect, visible, a=1") << false << true << 1.;
    QTest::newRow("effect, !visible, a=1") << true << false << 1.;
    QTest::newRow("effect, visible, a=1") << true << true << 1.;

    QTest::newRow("!effect, !visible, a=.5") << false << false << .5;
    QTest::newRow("!effect, visible, a=.5") << false << true << .5;
    QTest::newRow("effect, !visible, a=.5") << true << false << .5;
    QTest::newRow("effect, visible, a=.5") << true << true << .5;

    QTest::newRow("!effect, !visible, a=0") << false << false << 0.;
    QTest::newRow("!effect, visible, a=0") << false << true << 0.;
    QTest::newRow("effect, !visible, a=0") << true << false << 0.;
    QTest::newRow("effect, visible, a=0") << true << true << 0.;
}

void tst_QQuickItemLayer::layerVisibility()
{
    if (m_isMesaSoftwareRasterizer && m_mesaVersion < QT_VERSION_CHECK(7, 11, 0))
        QSKIP("Mesa Software Rasterizer below version 7.11 does not render this test correctly.");

    QFETCH(bool, visible);
    QFETCH(bool, effect);
    QFETCH(qreal, opacity);

    QQuickView view;
    view.setSource(testFileUrl("Visible.qml"));

    QQuickItem *child = view.contentItem()->childItems().at(0);
    child->setProperty("layerVisible", visible);
    child->setProperty("layerEffect", effect);
    child->setProperty("layerOpacity", opacity);

    view.show();

    QTest::qWaitForWindowExposed(&view);

    QImage fb = view.grabWindow();
    uint pixel = fb.pixel(0, 0);

    if (!visible || opacity == 0) {
        QCOMPARE(pixel, qRgb(0xff, 0xff, 0xff));
    } else if (effect) {
        QCOMPARE(qRed(pixel), 0xff);
        QVERIFY(qGreen(pixel) < 0xff);
        QVERIFY(qBlue(pixel) < 0xff);
    } else { // no effect
        QCOMPARE(qBlue(pixel), 0xff);
        QVERIFY(qGreen(pixel) < 0xff);
        QVERIFY(qRed(pixel) < 0xff);
    }
}




void tst_QQuickItemLayer::layerZOrder_data()
{
    QTest::addColumn<bool>("effect");

    QTest::newRow("!effect") << false;
    QTest::newRow("effect") << true;
}

void tst_QQuickItemLayer::layerZOrder()
{
    if (m_isMesaSoftwareRasterizer && m_mesaVersion < QT_VERSION_CHECK(7, 11, 0))
        QSKIP("Mesa Software Rasterizer below version 7.11 does not render this test correctly.");

    QFETCH(bool, effect);

    QQuickView view;
    view.setSource(testFileUrl("ZOrder.qml"));

    QQuickItem *child = view.contentItem()->childItems().at(0);
    child->setProperty("layerEffect", effect);

    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QImage fb = view.grabWindow();

    QCOMPARE(fb.pixel(50, 50), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(150, 150), qRgb(0, 0xff, 00));

}

void tst_QQuickItemLayer::changeZOrder_data()
{
    QTest::addColumn<bool>("layered");
    QTest::addColumn<bool>("effect");

    QTest::newRow("layered, effect") << true << true;
    QTest::newRow("layered, !effect") << true << false;
    QTest::newRow("!layered") << false << false;
}

void tst_QQuickItemLayer::changeZOrder()
{
    if (m_isMesaSoftwareRasterizer && m_mesaVersion < QT_VERSION_CHECK(7, 11, 0))
        QSKIP("Mesa Software Rasterizer below version 7.11 does not render this test correctly.");

    QFETCH(bool, layered);
    QFETCH(bool, effect);

    QQuickView view;
    view.setSource(testFileUrl("ZOrderChange.qml"));

    QQuickItem *child = view.contentItem()->childItems().at(0);
    child->setProperty("layerEnabled", layered);
    child->setProperty("layerEffect", effect);
    child->setProperty("layerZ", 1);

    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QImage fb = view.grabWindow();

    QRgb topLeft = fb.pixel(50, 50);
    QRgb topRight = fb.pixel(150, 50);
    QRgb bottomLeft = fb.pixel(50, 150);
    QRgb bottomRight = fb.pixel(150, 150);

    QCOMPARE(bottomLeft, qRgb(0, 0, 0xff));

    if (layered) {
        QCOMPARE(topLeft, qRgb(0, 0xff, 0xff));
    } else {
        QCOMPARE(qGreen(topLeft), 0xff);
        QVERIFY(qAbs(qRed(topLeft) - 0x3f) < 4);
        QVERIFY(qAbs(qBlue(topLeft) - 0xbf) < 4);
    }

    if (layered && effect) {
        QCOMPARE(qRed(topRight), 0xff);
        QCOMPARE(qGreen(topRight), 0x00);
        QVERIFY(qAbs(qBlue(topRight) - 0x7f) < 4);

        QVERIFY(qAbs(qRed(bottomRight) - 0x7f) < 4);
        QCOMPARE(qBlue(bottomRight), 0xff);
        QVERIFY(qAbs(qGreen(bottomRight) - 0x7f) < 4);
    } else {
        QCOMPARE(qRed(topRight), 0xff);
        QCOMPARE(qBlue(topRight), 0x00);
        QVERIFY(qAbs(qGreen(topRight) - 0x7f) < 4);

        QVERIFY(qAbs(qRed(bottomRight) - 0x7f) < 4);
        QCOMPARE(qGreen(bottomRight), 0xff);
        QVERIFY(qAbs(qBlue(bottomRight) - 0x7f) < 4);
    }
}

void tst_QQuickItemLayer::toggleLayerAndEffect()
{
    // This test passes if it doesn't crash.
    runTest("ToggleLayerAndEffect.qml");
}

void tst_QQuickItemLayer::disableLayer()
{
    // This test passes if it doesn't crash.
    runTest("DisableLayer.qml");
}

void tst_QQuickItemLayer::changeSamplerName()
{
    if (m_isMesaSoftwareRasterizer && m_mesaVersion < QT_VERSION_CHECK(7, 11, 0))
        QSKIP("Mesa Software Rasterizer below version 7.11 does not render this test correctly.");
    QImage fb = runTest("SamplerNameChange.qml");
    QCOMPARE(fb.pixel(0, 0), qRgb(0, 0, 0xff));
}

void tst_QQuickItemLayer::itemEffect()
{
    if (m_isMesaSoftwareRasterizer && m_mesaVersion < QT_VERSION_CHECK(7, 11, 0))
        QSKIP("Mesa Software Rasterizer below version 7.11 does not render this test correctly.");
    QImage fb = runTest("ItemEffect.qml");
    QCOMPARE(fb.pixel(0, 0), qRgb(0xff, 0, 0));
    QCOMPARE(fb.pixel(199, 0), qRgb(0xff, 0, 0));
    QCOMPARE(fb.pixel(0, 199), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(199, 199), qRgb(0, 0, 0xff));
}

void tst_QQuickItemLayer::rectangleEffect()
{
    QImage fb = runTest("RectangleEffect.qml");
    QCOMPARE(fb.pixel(0, 0), qRgb(0, 0xff, 0));
    QCOMPARE(fb.pixel(199, 0), qRgb(0, 0xff, 0));
    QCOMPARE(fb.pixel(0, 199), qRgb(0, 0xff, 0));
    QCOMPARE(fb.pixel(199, 199), qRgb(0, 0xff, 0));

    QCOMPARE(fb.pixel(100, 0), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(199, 100), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(100, 199), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(0, 100), qRgb(0, 0, 0xff));
}


QTEST_MAIN(tst_QQuickItemLayer)

#include "tst_qquickitemlayer.moc"
