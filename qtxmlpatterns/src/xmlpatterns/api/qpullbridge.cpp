/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtXmlPatterns module of the Qt Toolkit.
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

#include <QVariant>

#include "qabstractxmlnodemodel_p.h"
#include "qitemmappingiterator_p.h"
#include "qitem_p.h"
#include "qxmlname.h"
#include "qxmlquery_p.h"

#include "qpullbridge_p.h"

QT_BEGIN_NAMESPACE

using namespace QPatternist;

/*!
  \brief Bridges a QPatternist::SequenceIterator to QAbstractXmlPullProvider.
  \class QPatternist::PullBridge
  \internal
  \reentrant
  \ingroup xml-tools

  The approach of this class is rather straight forward since QPatternist::SequenceIterator
  and QAbstractXmlPullProvider are conceptually similar. While QPatternist::SequenceIterator only
  delivers top level items(since it's not an event stream, it's a list of items), PullBridge
  needs to recursively iterate the children of nodes too, which is achieved through the
  stack m_iterators.
 */

AbstractXmlPullProvider::Event PullBridge::next()
{
    m_index = m_iterators.top().second->next();

    if(!m_index.isNull())
    {
        Item item(m_index);

        if(item && item.isAtomicValue())
            m_current = AtomicValue;
        else
        {
            Q_ASSERT(item.isNode());

            switch(m_index.kind())
            {
                case QXmlNodeModelIndex::Attribute:
                {
                    m_current = Attribute;
                    break;
                }
                case QXmlNodeModelIndex::Comment:
                {
                    m_current = Comment;
                    break;
                }
                case QXmlNodeModelIndex::Element:
                {
                    m_iterators.push(qMakePair(StartElement, m_index.iterate(QXmlNodeModelIndex::AxisChild)));
                    m_current = StartElement;
                    break;
                }
                case QXmlNodeModelIndex::Document:
                {
                    m_iterators.push(qMakePair(StartDocument, m_index.iterate(QXmlNodeModelIndex::AxisChild)));
                    m_current = StartDocument;
                    break;
                }
                case QXmlNodeModelIndex::Namespace:
                {
                    m_current = Namespace;
                    break;
                }
                case QXmlNodeModelIndex::ProcessingInstruction:
                {
                    m_current = ProcessingInstruction;
                    break;
                }
                case QXmlNodeModelIndex::Text:
                {
                    m_current = Text;
                    break;
                }
            }
        }
    }
    else
    {
        if(m_iterators.isEmpty())
            m_current = EndOfInput;
        else
        {
            switch(m_iterators.top().first)
            {
                case StartOfInput:
                {
                    m_current = EndOfInput;
                    break;
                }
                case StartElement:
                {
                    m_current = EndElement;
                    m_iterators.pop();
                    break;
                }
                case StartDocument:
                {
                    m_current = EndDocument;
                    m_iterators.pop();
                    break;
                }
                default:
                {
                    Q_ASSERT_X(false, Q_FUNC_INFO,
                               "Invalid value.");
                    m_current = EndOfInput;
                }
            }
        }

    }

    return m_current;
}

AbstractXmlPullProvider::Event PullBridge::current() const
{
    return m_current;
}

QXmlNodeModelIndex PullBridge::index() const
{
    return m_index;
}

QSourceLocation PullBridge::sourceLocation() const
{
    return m_index.model()->sourceLocation(m_index);
}

QXmlName PullBridge::name() const
{
    return m_index.name();
}

QVariant PullBridge::atomicValue() const
{
    return QVariant();
}

QString PullBridge::stringValue() const
{
    return QString();
}

QHash<QXmlName, QString> PullBridge::attributes()
{
    Q_ASSERT(m_current == StartElement);

    QHash<QXmlName, QString> attributes;

    QXmlNodeModelIndex::Iterator::Ptr it = m_index.iterate(QXmlNodeModelIndex::AxisAttribute);
    QXmlNodeModelIndex index = it->next();
    while (!index.isNull()) {
        const Item attribute(index);
        attributes.insert(index.name(), index.stringValue());

        index = it->next();
    }

    return attributes;
}

QHash<QXmlName, QXmlItem> PullBridge::attributeItems()
{
    Q_ASSERT(m_current == StartElement);

    QHash<QXmlName, QXmlItem> attributes;

    QXmlNodeModelIndex::Iterator::Ptr it = m_index.iterate(QXmlNodeModelIndex::AxisAttribute);
    QXmlNodeModelIndex index = it->next();
    while (!index.isNull()) {
        const Item attribute(index);
        attributes.insert(index.name(), QXmlItem(index));

        index = it->next();
    }

    return attributes;
}

QT_END_NAMESPACE

