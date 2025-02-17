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

#include "qxsdvalidatedxmlnodemodel_p.h"

#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE

using namespace QPatternist;

XsdValidatedXmlNodeModel::XsdValidatedXmlNodeModel(const QAbstractXmlNodeModel *model)
    : m_internalModel(model)
{
}

XsdValidatedXmlNodeModel::~XsdValidatedXmlNodeModel()
{
}

QUrl XsdValidatedXmlNodeModel::baseUri(const QXmlNodeModelIndex &index) const
{
    return m_internalModel->baseUri(index);
}

QUrl XsdValidatedXmlNodeModel::documentUri(const QXmlNodeModelIndex &index) const
{
    return m_internalModel->documentUri(index);
}

QXmlNodeModelIndex::NodeKind XsdValidatedXmlNodeModel::kind(const QXmlNodeModelIndex &index) const
{
    return m_internalModel->kind(index);
}

QXmlNodeModelIndex::DocumentOrder XsdValidatedXmlNodeModel::compareOrder(const QXmlNodeModelIndex &index, const QXmlNodeModelIndex &otherIndex) const
{
    return m_internalModel->compareOrder(index, otherIndex);
}

QXmlNodeModelIndex XsdValidatedXmlNodeModel::root(const QXmlNodeModelIndex &index) const
{
    return m_internalModel->root(index);
}

QXmlName XsdValidatedXmlNodeModel::name(const QXmlNodeModelIndex &index) const
{
    return m_internalModel->name(index);
}

QString XsdValidatedXmlNodeModel::stringValue(const QXmlNodeModelIndex &index) const
{
    return m_internalModel->stringValue(index);
}

QVariant XsdValidatedXmlNodeModel::typedValue(const QXmlNodeModelIndex &index) const
{
    return m_internalModel->typedValue(index);
}

QExplicitlySharedDataPointer<QAbstractXmlForwardIterator<QXmlNodeModelIndex> > XsdValidatedXmlNodeModel::iterate(const QXmlNodeModelIndex &index, QXmlNodeModelIndex::Axis axis) const
{
    return m_internalModel->iterate(index, axis);
}

QPatternist::ItemIteratorPtr XsdValidatedXmlNodeModel::sequencedTypedValue(const QXmlNodeModelIndex &index) const
{
    return m_internalModel->sequencedTypedValue(index);
}

QPatternist::ItemTypePtr XsdValidatedXmlNodeModel::type(const QXmlNodeModelIndex &index) const
{
    return m_internalModel->type(index);
}

QXmlName::NamespaceCode XsdValidatedXmlNodeModel::namespaceForPrefix(const QXmlNodeModelIndex &index, const QXmlName::PrefixCode prefix) const
{
    return m_internalModel->namespaceForPrefix(index, prefix);
}

bool XsdValidatedXmlNodeModel::isDeepEqual(const QXmlNodeModelIndex &index, const QXmlNodeModelIndex &otherIndex) const
{
    return m_internalModel->isDeepEqual(index, otherIndex);
}

void XsdValidatedXmlNodeModel::sendNamespaces(const QXmlNodeModelIndex &index, QAbstractXmlReceiver *const receiver) const
{
    m_internalModel->sendNamespaces(index, receiver);
}

QVector<QXmlName> XsdValidatedXmlNodeModel::namespaceBindings(const QXmlNodeModelIndex &index) const
{
    return m_internalModel->namespaceBindings(index);
}

QXmlNodeModelIndex XsdValidatedXmlNodeModel::elementById(const QXmlName &name) const
{
    return m_internalModel->elementById(name);
}

QVector<QXmlNodeModelIndex> XsdValidatedXmlNodeModel::nodesByIdref(const QXmlName &name) const
{
    return m_internalModel->nodesByIdref(name);
}

void XsdValidatedXmlNodeModel::copyNodeTo(const QXmlNodeModelIndex &index, QAbstractXmlReceiver *const receiver, const NodeCopySettings &settings) const
{
    return m_internalModel->copyNodeTo(index, receiver, settings);
}

QXmlNodeModelIndex XsdValidatedXmlNodeModel::nextFromSimpleAxis(SimpleAxis axis, const QXmlNodeModelIndex &origin) const
{
    return m_internalModel->nextFromSimpleAxis(axis, origin);
}

QVector<QXmlNodeModelIndex> XsdValidatedXmlNodeModel::attributes(const QXmlNodeModelIndex &index) const
{
    return m_internalModel->attributes(index);
}

void XsdValidatedXmlNodeModel::setAssignedElement(const QXmlNodeModelIndex &index, const XsdElement::Ptr &element)
{
    m_assignedElements.insert(index, element);
}

XsdElement::Ptr XsdValidatedXmlNodeModel::assignedElement(const QXmlNodeModelIndex &index) const
{
    if (m_assignedElements.contains(index))
        return m_assignedElements.value(index);
    else
        return XsdElement::Ptr();
}

void XsdValidatedXmlNodeModel::setAssignedAttribute(const QXmlNodeModelIndex &index, const XsdAttribute::Ptr &attribute)
{
    m_assignedAttributes.insert(index, attribute);
}

XsdAttribute::Ptr XsdValidatedXmlNodeModel::assignedAttribute(const QXmlNodeModelIndex &index) const
{
    if (m_assignedAttributes.contains(index))
        return m_assignedAttributes.value(index);
    else
        return XsdAttribute::Ptr();
}

void XsdValidatedXmlNodeModel::setAssignedType(const QXmlNodeModelIndex &index, const SchemaType::Ptr &type)
{
    m_assignedTypes.insert(index, type);
}

SchemaType::Ptr XsdValidatedXmlNodeModel::assignedType(const QXmlNodeModelIndex &index) const
{
    if (m_assignedTypes.contains(index))
        return m_assignedTypes.value(index);
    else
        return SchemaType::Ptr();
}

void XsdValidatedXmlNodeModel::addIdIdRefBinding(const QString &id, const NamedSchemaComponent::Ptr &binding)
{
    m_idIdRefBindings[id].insert(binding);
}

QStringList XsdValidatedXmlNodeModel::idIdRefBindingIds() const
{
    return m_idIdRefBindings.keys();
}

QSet<NamedSchemaComponent::Ptr> XsdValidatedXmlNodeModel::idIdRefBindings(const QString &id) const
{
    return m_idIdRefBindings.value(id);
}

QT_END_NAMESPACE
