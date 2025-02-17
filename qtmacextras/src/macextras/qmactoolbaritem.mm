/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtMacExtras module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmactoolbaritem.h"
#include "qmactoolbaritem_p.h"
#include "qmacfunctions.h"
#include "qmacfunctions_p.h"
#include "qmactoolbar_p.h"

QT_BEGIN_NAMESPACE

/*!
  \class QMacToolBarItem
  \inmodule QtMacExtras
  \since 5.3
  \brief The QMacToolBarItem class provides an item for QMacToolBar.

  All items should have the text and icon properites set, or have the
  standardItem property set to something else than NoStandardItem.

  \sa QMacToolBar
*/

/*!
    \enum QMacToolBarItem::StandardItem
    \value NoStandardItem Don't use a standard item
    \value Space A spacing item
    \value FlexibleSpace A spacing item which grows to fill available space
*/


/*!
    Constructs a QMacToolBarItem with \a parent.
*/
QMacToolBarItem::QMacToolBarItem(QObject *parent)
    :QObject(*new QMacToolBarItemPrivate, parent)
{
    Q_D(QMacToolBarItem);
    d->initNativeToolbarItem();
}

/*!
    Destroys a QMacToolBarItem
*/
QMacToolBarItem::~QMacToolBarItem()
{

}

/*!
    \property QMacToolBarItem::selectable
    \brief Whether the item is selecatble

    This property's default is false.
*/
bool QMacToolBarItem::selectable() const
{
    Q_D(const QMacToolBarItem);
    return d->selectable;
}

void QMacToolBarItem::setSelectable(bool selectable)
{
    Q_D(QMacToolBarItem);
    d->selectable = selectable;
}

/*!
    \property QMacToolBarItem::standardItem
    \brief Whether the item is a standard item.

    This property's default is NoStandardItem, in which case the icon and text
    property determines the item contents.

    Setting this property to somthing else than NoStandardItem takes precendense
    over icon and text.
*/
QMacToolBarItem::StandardItem QMacToolBarItem::standardItem() const
{
    Q_D(const QMacToolBarItem);
    return d->standardItem;
}

void QMacToolBarItem::setStandardItem(StandardItem standardItem)
{
    Q_D(QMacToolBarItem);

    if (d->standardItem == standardItem)
        return;

    d->standardItem = standardItem;
    d->initNativeToolbarItem();
}

/*!
    \property QMacToolBarItem::text
    \brief The item's text.
*/
QString QMacToolBarItem::text() const
{
    Q_D(const QMacToolBarItem);
    return d->text;
}

void QMacToolBarItem::setText(const QString &text)
{
    Q_D(QMacToolBarItem);
    d->text = text;

    if (d->standardItem != QMacToolBarItem::NoStandardItem)
        return;

    [d->toolbarItem setLabel:text.toNSString()];
}

/*!
    \property QMacToolBarItem::icon
    \brief The item's icon.
*/
QIcon QMacToolBarItem::icon() const
{
    Q_D(const QMacToolBarItem);
    return d->icon;
}

void QMacToolBarItem::setIcon(const QIcon &icon)
{
    Q_D(QMacToolBarItem);
    d->icon = icon;
    QPixmap pixmap = icon.pixmap(64, 64);

    if (d->standardItem != QMacToolBarItem::NoStandardItem)
        return;

    if (pixmap.isNull() == false) {
        [d->toolbarItem setImage: QtMac::toNSImage(pixmap)];
    }
}

/*!
    Returns the native NSToolbarItem.
*/
NSToolbarItem *QMacToolBarItem::nativeToolBarItem() const
{
    Q_D(const QMacToolBarItem);
    return d->toolbarItem;
}

/*!
    \fn void QMacToolBarItem::activated();

    This signal is emitted when the toolbar item is clicked or otherwise activated.
*/

QMacToolBarItemPrivate::QMacToolBarItemPrivate()
{
    standardItem = QMacToolBarItem::NoStandardItem;
    selectable = false;
    toolbarItem = 0;
}

QMacToolBarItemPrivate::~QMacToolBarItemPrivate()
{
    [toolbarItem release];
}

NSString *QMacToolBarItemPrivate::toNSStandardItem(QMacToolBarItem::StandardItem standardItem)
{
    /*
        ### not supported yet; Qt does not handle the action
        messages these send.

    if (standardItem == QMacToolBarItem::ShowColors)
        return NSToolbarShowColorsItemIdentifier;
    else if (standardItem == QMacToolBarItem::ShowFonts)
        return NSToolbarShowFontsItemIdentifier;
    else if (standardItem == QMacToolBarItem::PrintItem)
        return NSToolbarPrintItemIdentifier;
    else
*/
    if (standardItem == QMacToolBarItem::Space)
        return NSToolbarSpaceItemIdentifier;
    else if (standardItem == QMacToolBarItem::FlexibleSpace)
        return NSToolbarFlexibleSpaceItemIdentifier;
    return @"";
}

NSString *QMacToolBarItemPrivate::itemIdentifier() const
{
    if (standardItem == QMacToolBarItem::NoStandardItem)
        return QString::number(qulonglong(q_func())).toNSString();

    return toNSStandardItem(standardItem);
}

// Initializes the NSToolBarItem based on QMacToolBarItem properties.
void QMacToolBarItemPrivate::initNativeToolbarItem()
{
    NSString  *identifier = itemIdentifier();
    toolbarItem = [[NSToolbarItem alloc] initWithItemIdentifier:identifier];
}

QT_END_NAMESPACE
