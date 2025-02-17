/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtLocation module of the Qt Toolkit.
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
#ifndef QPLACECONTENT_H
#define QPLACECONTENT_H

#include <QtLocation/qlocationglobal.h>

#include <QMap>
#include <QMetaType>
#include <QSharedDataPointer>

QT_BEGIN_NAMESPACE

#define Q_DECLARE_CONTENT_D_FUNC(Class) \
    inline Class##Private *d_func(); \
    inline const Class##Private *d_func() const;\
    friend class Class##Private;

#define Q_DECLARE_CONTENT_COPY_CTOR(Class) \
    Class(const QPlaceContent &other);

class QPlaceUser;
class QPlaceSupplier;
class QPlaceContentPrivate;
class Q_LOCATION_EXPORT QPlaceContent
{
public:
    typedef QMap<int, QPlaceContent> Collection;

    enum Type {
        NoType = 0,
        ImageType,
        ReviewType,
        EditorialType
    };

    QPlaceContent();
    QPlaceContent(const QPlaceContent &other);
    virtual ~QPlaceContent();

    QPlaceContent &operator=(const QPlaceContent &other);

    bool operator==(const QPlaceContent &other) const;
    bool operator!=(const QPlaceContent &other) const;

    QPlaceContent::Type type() const;

    QPlaceSupplier supplier() const;
    void setSupplier(const QPlaceSupplier &supplier);

    QPlaceUser user() const;
    void setUser(const QPlaceUser &user);

    QString attribution() const;
    void setAttribution(const QString &attribution);

protected:
    explicit QPlaceContent(QPlaceContentPrivate *d);
    QSharedDataPointer<QPlaceContentPrivate> d_ptr;

private:
    inline QPlaceContentPrivate *d_func();
    inline const QPlaceContentPrivate *d_func() const;

    friend class QPlaceContentPrivate;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QPlaceContent)
Q_DECLARE_METATYPE(QPlaceContent::Type)

#endif

