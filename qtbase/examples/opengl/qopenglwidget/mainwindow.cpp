/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"

#include <QApplication>
#include <QMenuBar>
#include <QGroupBox>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>

#include "glwidget.h"

MainWindow::MainWindow()
    : m_nextX(1), m_nextY(1)
{
    GLWidget *glwidget = new GLWidget(this, true, qRgb(20, 20, 50));
    m_glWidgets << glwidget;
    QLabel *label = new QLabel(this);
    m_timer = new QTimer(this);
    QSlider *slider = new QSlider(this);
    slider->setOrientation(Qt::Horizontal);

    QLabel *updateLabel = new QLabel("Update interval");
    QSpinBox *updateInterval = new QSpinBox(this);
    updateInterval->setSuffix(" ms");
    updateInterval->setValue(10);
    updateInterval->setToolTip("Interval for the timer that calls update().\n"
                               "Note that on most systems the swap will block to wait for vsync\n"
                               "and therefore an interval < 16 ms will likely lead to a 60 FPS update rate.");
    QGroupBox *updateGroupBox = new QGroupBox(this);
    QCheckBox *timerBased = new QCheckBox("Use timer", this);
    timerBased->setChecked(true);
    timerBased->setToolTip("Toggles using a timer to trigger update().\n"
                           "When not set, each paintGL() schedules the next update immediately,\n"
                           "expecting the blocking swap to throttle the thread.\n"
                           "This shows how unnecessary the timer is in most cases.");
    QCheckBox *transparent = new QCheckBox("Transparent background", this);
    transparent->setToolTip("Toggles Qt::WA_AlwaysStackOnTop and transparent clear color for glClear().\n"
                            "Note how the button on top stacks incorrectly when enabling this.");
    QHBoxLayout *updateLayout = new QHBoxLayout;
    updateLayout->addWidget(updateLabel);
    updateLayout->addWidget(updateInterval);
    updateLayout->addWidget(timerBased);
    updateLayout->addWidget(transparent);
    updateGroupBox->setLayout(updateLayout);

    slider->setRange(0, 50);
    slider->setSliderPosition(30);
    m_timer->setInterval(10);
    label->setText("A QOpenGLWidget");
    label->setAlignment(Qt::AlignHCenter);

    QGroupBox * groupBox = new QGroupBox(this);
    setCentralWidget(groupBox);
    groupBox->setTitle("QOpenGLWidget Example");

    m_layout = new QGridLayout(groupBox);

    m_layout->addWidget(glwidget,1,0,8,1);
    m_layout->addWidget(label,9,0,1,1);
    m_layout->addWidget(updateGroupBox, 10, 0, 1, 1);
    m_layout->addWidget(slider, 11,0,1,1);

    groupBox->setLayout(m_layout);

    QMenu *fileMenu = menuBar()->addMenu("&File");
    QMenu *showMenu = menuBar()->addMenu("&Show");
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *exit = new QAction("E&xit", fileMenu);
    QAction *aboutQt = new QAction("About Qt", helpMenu);
    QAction *showLogo = new QAction("Show 3D Logo", showMenu);
    QAction *showTexture = new QAction("Show 2D Texture", showMenu);
    QAction *showBubbles = new QAction("Show bubbles", showMenu);
    showBubbles->setCheckable(true);
    showBubbles->setChecked(true);
    fileMenu->addAction(exit);
    helpMenu->addAction(aboutQt);
    showMenu->addAction(showLogo);
    showMenu->addAction(showTexture);
    showMenu->addAction(showBubbles);

    connect(exit, SIGNAL(triggered(bool)), this, SLOT(close()));
    connect(aboutQt, SIGNAL(triggered(bool)), qApp, SLOT(aboutQt()));

    connect(m_timer, SIGNAL(timeout()), glwidget, SLOT(update()));

    connect(showLogo, SIGNAL(triggered(bool)), glwidget, SLOT(setLogo()));
    connect(showTexture, SIGNAL(triggered(bool)), glwidget, SLOT(setTexture()));
    connect(showBubbles, SIGNAL(triggered(bool)), glwidget, SLOT(setShowBubbles(bool)));
    connect(slider, SIGNAL(valueChanged(int)), glwidget, SLOT(setScaling(int)));
    connect(transparent, &QCheckBox::toggled, glwidget, &GLWidget::setTransparent);

    connect(updateInterval, SIGNAL(valueChanged(int)), this, SLOT(updateIntervalChanged(int)));
    connect(timerBased, &QCheckBox::toggled, this, &MainWindow::timerUsageChanged);
    connect(timerBased, &QCheckBox::toggled, updateInterval, &QWidget::setEnabled);

    m_timer->start();
}

void MainWindow::updateIntervalChanged(int value)
{
    m_timer->setInterval(value);
    if (m_timer->isActive())
        m_timer->start();
}

void MainWindow::addNew()
{
    if (m_nextY == 4)
        return;
    GLWidget *w = new GLWidget(this, false, qRgb(qrand() % 256, qrand() % 256, qrand() % 256));
    m_glWidgets << w;
    connect(m_timer, SIGNAL(timeout()), w, SLOT(update()));
    m_layout->addWidget(w, m_nextY, m_nextX, 1, 1);
    if (m_nextX == 3) {
        m_nextX = 1;
        ++m_nextY;
    } else {
        ++m_nextX;
    }
}

void MainWindow::timerUsageChanged(bool enabled)
{
    if (enabled) {
        m_timer->start();
    } else {
        m_timer->stop();
        foreach (QOpenGLWidget *w, m_glWidgets)
            w->update();
    }
}
