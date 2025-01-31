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

#include "imageitem.h"
#include "gestures.h"

#include <QPainter>
#include <QEvent>

ImageItem::ImageItem(const QImage &image)
{
    setImage(image);
}

void ImageItem::setImage(const QImage &image)
{
    image_ = image;
    pixmap_ = QPixmap::fromImage(image.scaled(400, 400, Qt::KeepAspectRatio));
    update();
}

QImage ImageItem::image() const
{
    return image_;
}

QRectF ImageItem::boundingRect() const
{
    const QSize size = pixmap_.size();
    return QRectF(0, 0, size.width(), size.height());
}

void ImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->drawPixmap(0, 0, pixmap_);
}


GestureImageItem::GestureImageItem(const QImage &image)
    : ImageItem(image)
{
    grabGesture(Qt::PanGesture);
    grabGesture(ThreeFingerSlideGesture::Type);
}

bool GestureImageItem::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture) {
        qDebug("gestureimageitem: gesture triggered");
        return true;
    }
    return ImageItem::event(event);
}

#include "moc_imageitem.cpp"
