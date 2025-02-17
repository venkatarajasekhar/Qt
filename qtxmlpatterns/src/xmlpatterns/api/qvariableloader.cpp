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
#include <QStringList>

#include "qanyuri_p.h"
#include "qatomicstring_p.h"
#include "qbuiltintypes_p.h"
#include "qcommonsequencetypes_p.h"
#include "qgenericsequencetype_p.h"
#include "qinteger_p.h"
#include "qitem_p.h"
#include "qsequencetype_p.h"
#include "qvariableloader_p.h"
#include "qxmlquery_p.h"

QT_BEGIN_NAMESPACE

namespace QPatternist
{

    class VariantListIterator : public ListIteratorPlatform<QVariant, Item, VariantListIterator>
    {
    public:
        inline VariantListIterator(const QVariantList &list) : ListIteratorPlatform<QVariant, Item, VariantListIterator>(list)
        {
        }

    private:
        friend class ListIteratorPlatform<QVariant, Item, VariantListIterator>;

        inline Item inputToOutputItem(const QVariant &inputType) const
        {
            return AtomicValue::toXDM(inputType);
        }
    };

    class StringListIterator : public ListIteratorPlatform<QString, Item, StringListIterator>
    {
    public:
        inline StringListIterator(const QStringList &list) : ListIteratorPlatform<QString, Item, StringListIterator>(list)
        {
        }

    private:
        friend class ListIteratorPlatform<QString, Item, StringListIterator>;

        static inline Item inputToOutputItem(const QString &inputType)
        {
            return AtomicString::fromValue(inputType);
        }
    };

    /**
     * Takes two DynamicContext instances, and redirects the storage of temporary trees
     * to one of them.
     *
     * @since 4.5
     */
    class TemporaryTreesRedirectingContext : public DelegatingDynamicContext
    {
    public:
        TemporaryTreesRedirectingContext(const DynamicContext::Ptr &other,
                                         const DynamicContext::Ptr &modelStorage) : DelegatingDynamicContext(other)
                                                                                  , m_modelStorage(modelStorage)
        {
            Q_ASSERT(m_modelStorage);
        }

        virtual void addNodeModel(const QAbstractXmlNodeModel::Ptr &nodeModel)
        {
            m_modelStorage->addNodeModel(nodeModel);
        }

    private:
        const DynamicContext::Ptr m_modelStorage;
    };
}

using namespace QPatternist;

SequenceType::Ptr VariableLoader::announceExternalVariable(const QXmlName name,
                                                           const SequenceType::Ptr &declaredType)
{
    Q_UNUSED(declaredType);
    const QVariant &variant = m_bindingHash.value(name);


    if(variant.isNull())
        return SequenceType::Ptr();
    else if(variant.userType() == qMetaTypeId<QIODevice *>())
        return CommonSequenceTypes::ExactlyOneAnyURI;
    else if(variant.userType() == qMetaTypeId<QXmlQuery>())
    {
        const QXmlQuery variableQuery(qvariant_cast<QXmlQuery>(variant));
        return variableQuery.d->expression()->staticType();
    }
    else
    {
        return makeGenericSequenceType(AtomicValue::qtToXDMType(qvariant_cast<QXmlItem>(variant)),
                                       Cardinality::exactlyOne());
    }
}

Item::Iterator::Ptr VariableLoader::evaluateSequence(const QXmlName name,
                                                     const DynamicContext::Ptr &context)
{

    const QVariant &variant = m_bindingHash.value(name);
    Q_ASSERT_X(!variant.isNull(), Q_FUNC_INFO,
               "We assume that we have a binding.");

    /* Same code as in the default clause below. */
    if(variant.userType() == qMetaTypeId<QIODevice *>())
        return makeSingletonIterator(itemForName(name));
    else if(variant.userType() == qMetaTypeId<QXmlQuery>())
    {
        const QXmlQuery variableQuery(qvariant_cast<QXmlQuery>(variant));

        return variableQuery.d->expression()->evaluateSequence(DynamicContext::Ptr(new TemporaryTreesRedirectingContext(variableQuery.d->dynamicContext(), context)));
    }

    const QVariant v(qvariant_cast<QXmlItem>(variant).toAtomicValue());

    switch(v.type())
    {
        case QVariant::StringList:
            return Item::Iterator::Ptr(new StringListIterator(v.toStringList()));
        case QVariant::List:
            return Item::Iterator::Ptr(new VariantListIterator(v.toList()));
        default:
            return makeSingletonIterator(itemForName(name));
    }
}

Item VariableLoader::itemForName(const QXmlName &name) const
{
    const QVariant &variant = m_bindingHash.value(name);

    if(variant.userType() == qMetaTypeId<QIODevice *>())
        return Item(AnyURI::fromValue(QLatin1String("tag:trolltech.com,2007:QtXmlPatterns:QIODeviceVariable:") + m_namePool->stringForLocalName(name.localName())));

    const QXmlItem item(qvariant_cast<QXmlItem>(variant));

    if(item.isNode())
        return Item::fromPublic(item);
    else
    {
        const QVariant atomicValue(item.toAtomicValue());
        /* If the atomicValue is null it means it doesn't exist in m_bindingHash, and therefore it must
         * be a QIODevice, since Patternist guarantees to only ask for variables that announceExternalVariable()
         * has accepted. */
        if(atomicValue.isNull())
            return Item(AnyURI::fromValue(QLatin1String("tag:trolltech.com,2007:QtXmlPatterns:QIODeviceVariable:") + m_namePool->stringForLocalName(name.localName())));
        else
            return AtomicValue::toXDM(atomicValue);
    }
}

Item VariableLoader::evaluateSingleton(const QXmlName name,
                                       const DynamicContext::Ptr &)
{
    return itemForName(name);
}

bool VariableLoader::isSameType(const QVariant &v1,
                                const QVariant &v2) const
{
    /* Are both of type QIODevice *? */
    if(v1.userType() == qMetaTypeId<QIODevice *>() && v1.userType() == v2.userType())
        return true;

    /* Ok, we have two QXmlItems. */
    const QXmlItem i1(qvariant_cast<QXmlItem>(v1));
    const QXmlItem i2(qvariant_cast<QXmlItem>(v2));

    if(i1.isNode())
    {
        Q_ASSERT(false);
        return false;
    }
    else if(i2.isAtomicValue())
        return i1.toAtomicValue().type() == i2.toAtomicValue().type();
    else
    {
        /* One is an atomic, the other is a node or they are null. */
        return false;
    }
}

void VariableLoader::removeBinding(const QXmlName &name)
{
    m_bindingHash.remove(name);
}

bool VariableLoader::hasBinding(const QXmlName &name) const
{
    return m_bindingHash.contains(name)
        || (m_previousLoader && m_previousLoader->hasBinding(name));
}

QVariant VariableLoader::valueFor(const QXmlName &name) const
{
    if(m_bindingHash.contains(name))
        return m_bindingHash.value(name);
    else if(m_previousLoader)
        return m_previousLoader->valueFor(name);
    else
        return QVariant();
}

void VariableLoader::addBinding(const QXmlName &name,
                                const QVariant &value)
{
    m_bindingHash.insert(name, value);
}

bool VariableLoader::invalidationRequired(const QXmlName &name,
                                          const QVariant &variant) const
{
    return hasBinding(name) && !isSameType(valueFor(name), variant);
}

QT_END_NAMESPACE

