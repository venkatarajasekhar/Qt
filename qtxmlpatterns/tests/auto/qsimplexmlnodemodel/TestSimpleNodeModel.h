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


#ifndef TestSimpleNodeModel_h
#define TestSimpleNodeModel_h

#include <QtXmlPatterns/QSimpleXmlNodeModel>
#include <QtXmlPatterns/QXmlNamePool>

class TestSimpleNodeModel : public QSimpleXmlNodeModel
{
public:

    TestSimpleNodeModel(const QXmlNamePool &np);

    QXmlNodeModelIndex root() const;

    /*!
      This function is made public, such that we can test its const qualification.
     */
    virtual QXmlNodeModelIndex nextFromSimpleAxis(SimpleAxis axis,
                                                  const QXmlNodeModelIndex &origin) const;

    /*!
      This function is made public, such that we can test its const qualification.
     */
    virtual QVector<QXmlNodeModelIndex> attributes(const QXmlNodeModelIndex&) const;

    virtual QUrl documentUri(const QXmlNodeModelIndex&) const;
    virtual QXmlNodeModelIndex::NodeKind kind(const QXmlNodeModelIndex&) const;
    virtual QXmlNodeModelIndex::DocumentOrder compareOrder(const QXmlNodeModelIndex&,
                                                           const QXmlNodeModelIndex&) const;
    virtual QXmlNodeModelIndex root(const QXmlNodeModelIndex&) const;
    virtual QXmlName name(const QXmlNodeModelIndex&) const;
    virtual QVariant typedValue(const QXmlNodeModelIndex&) const;
};

TestSimpleNodeModel::TestSimpleNodeModel(const QXmlNamePool &np) : QSimpleXmlNodeModel(np)
{

    /* If this fails to compile, QSimpleXmlNodeModel::namePool()
     * does not return a mutable reference. */
    QXmlName(namePool(), QLatin1String("name"));
}

QXmlNodeModelIndex TestSimpleNodeModel::root() const
{
    return createIndex(qint64(0), 1);
}

QXmlNodeModelIndex TestSimpleNodeModel::nextFromSimpleAxis(SimpleAxis, const QXmlNodeModelIndex &) const
{
    return QXmlNodeModelIndex();
}

QVector<QXmlNodeModelIndex> TestSimpleNodeModel::attributes(const QXmlNodeModelIndex&) const
{
    return QVector<QXmlNodeModelIndex>();
}

QUrl TestSimpleNodeModel::documentUri(const QXmlNodeModelIndex &) const
{
    return QUrl();
}

QXmlNodeModelIndex::NodeKind TestSimpleNodeModel::kind(const QXmlNodeModelIndex &) const
{
    return QXmlNodeModelIndex::Element;
}

QXmlNodeModelIndex::DocumentOrder TestSimpleNodeModel::compareOrder(const QXmlNodeModelIndex &,
                                                                    const QXmlNodeModelIndex &) const
{
    return QXmlNodeModelIndex::Precedes;
}

QXmlNodeModelIndex TestSimpleNodeModel::root(const QXmlNodeModelIndex &) const
{
    return QXmlNodeModelIndex();
}

QXmlName TestSimpleNodeModel::name(const QXmlNodeModelIndex &) const
{
    QXmlNamePool np(namePool());
    return QXmlName(np, QLatin1String("nodeName"));
}

QVariant TestSimpleNodeModel::typedValue(const QXmlNodeModelIndex&) const
{
    return QVariant();
}

#endif
