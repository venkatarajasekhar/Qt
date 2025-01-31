/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtPositioning module of the Qt Toolkit.
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
#include "widget.h"
#include "ui_widget.h"
#include <QGeoPositionInfoSource>
#include <QDebug>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    qDebug() << "Available:" << QGeoPositionInfoSource::availableSources();
    m_posSource = QGeoPositionInfoSource::createDefaultSource(this);
    if (!m_posSource)
        qFatal("No Position Source created!");
    connect(m_posSource, SIGNAL(positionUpdated(QGeoPositionInfo)),
            this, SLOT(positionUpdated(QGeoPositionInfo)));

    connect(ui->horizontalSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setInterval(int)));
    connect(m_posSource, SIGNAL(updateTimeout()),
            this, SLOT(positionTimedOut()));

    ui->groupBox->setLayout(ui->gridLayout);
    ui->horizontalSlider->setMinimum(m_posSource->minimumUpdateInterval());
    ui->labelTimeOut->setVisible(false);

    connect(m_posSource, SIGNAL(error(QGeoPositionInfoSource::Error)),
            this, SLOT(errorChanged(QGeoPositionInfoSource::Error)));
}

void Widget::positionUpdated(QGeoPositionInfo gpsPos)
{
    QGeoCoordinate coord = gpsPos.coordinate();
    ui->labelLatitude->setText(QString::number(coord.latitude()));
    ui->labelLongitude->setText(QString::number(coord.longitude()));
    ui->labelAltitude->setText(QString::number(coord.altitude()));
    ui->labelTimeStamp->setText(gpsPos.timestamp().toString());
    if (gpsPos.hasAttribute(QGeoPositionInfo::HorizontalAccuracy))
        ui->labelHAccuracy->setText(QString::number(gpsPos.attribute(QGeoPositionInfo::HorizontalAccuracy)));
    else
        ui->labelHAccuracy->setText(QStringLiteral("N/A"));

    if (gpsPos.hasAttribute(QGeoPositionInfo::VerticalAccuracy))
        ui->labelVAccuracy->setText(QString::number(gpsPos.attribute(QGeoPositionInfo::VerticalAccuracy)));
    else
        ui->labelVAccuracy->setText(QStringLiteral("N/A"));
}

void Widget::positionTimedOut()
{
    ui->labelTimeOut->setVisible(true);
}

void Widget::errorChanged(QGeoPositionInfoSource::Error err)
{
    ui->labelErrorState->setText(err == 3 ? QStringLiteral("OK") : QString::number(err));
}

Widget::~Widget()
{
    delete ui;
}

void Widget::setInterval(int msec)
{
    m_posSource->setUpdateInterval(msec);
}

void Widget::on_buttonRetrieve_clicked()
{
    // Requesting current position for _one_ time
    m_posSource->requestUpdate(10000);
}

void Widget::on_buttonStart_clicked()
{
    // Either start or stop the current position info source
    bool running = ui->checkBox->isChecked();
    if (running) {
        m_posSource->stopUpdates();
        ui->checkBox->setChecked(false);
    } else {
        m_posSource->startUpdates();
        ui->checkBox->setChecked(true);
    }
}

void Widget::on_radioButton_clicked()
{
    m_posSource->setPreferredPositioningMethods(QGeoPositionInfoSource::NoPositioningMethods);
}

void Widget::on_radioButton_2_clicked()
{
    m_posSource->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
}

void Widget::on_radioButton_3_clicked()
{
    m_posSource->setPreferredPositioningMethods(QGeoPositionInfoSource::NonSatellitePositioningMethods);
}

void Widget::on_radioButton_4_clicked()
{
    m_posSource->setPreferredPositioningMethods(QGeoPositionInfoSource::AllPositioningMethods);
}

void Widget::on_buttonUpdateSupported_clicked()
{
    QGeoPositionInfoSource::PositioningMethods m = m_posSource->supportedPositioningMethods();
    QString text;
    switch (m) {
    case QGeoPositionInfoSource::NoPositioningMethods:
        text = QStringLiteral("None");
        break;
    case QGeoPositionInfoSource::SatellitePositioningMethods:
        text = QStringLiteral("Satellite");
        break;
    case QGeoPositionInfoSource::NonSatellitePositioningMethods:
        text = QStringLiteral("Non Satellite");
        break;
    case QGeoPositionInfoSource::AllPositioningMethods:
        text = QStringLiteral("All");
        break;
    }

    ui->labelSupported->setText(text);
}
