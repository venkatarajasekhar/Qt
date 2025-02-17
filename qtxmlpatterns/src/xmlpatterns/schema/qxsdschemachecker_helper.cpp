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

QT_BEGIN_NAMESPACE

using namespace QPatternist;

bool XsdSchemaChecker::hasDuplicatedAttributeUses(const XsdAttributeUse::List &list, XsdAttribute::Ptr &conflictingAttribute) const
{
    const int length = list.count();

    for (int i = 0; i < length; ++i) {
        for (int j = 0; j < length; ++j) {
            if (i == j)
                continue;

            if (list.at(i)->attribute()->name(m_namePool) == list.at(j)->attribute()->name(m_namePool)) {
                conflictingAttribute = list.at(i)->attribute();
                return true;
            }
        }
    }

    return false;
}

bool XsdSchemaChecker::hasMultipleIDAttributeUses(const XsdAttributeUse::List &list) const
{
    const int length = list.count();

    bool hasIdDerivedAttribute = false;
    for (int i = 0; i < length; ++i) {
        if (BuiltinTypes::xsID->wxsTypeMatches(list.at(i)->attribute()->type())) {
            if (hasIdDerivedAttribute)
                return true;
            else
                hasIdDerivedAttribute = true;
        }
    }

    return false;
}

bool XsdSchemaChecker::hasConstraintIDAttributeUse(const XsdAttributeUse::List &list, XsdAttribute::Ptr &conflictingAttribute) const
{
    const int length = list.count();

    for (int i = 0; i < length; ++i) {
        const XsdAttributeUse::Ptr attributeUse(list.at(i));
        if (BuiltinTypes::xsID->wxsTypeMatches(attributeUse->attribute()->type())) {
            if (attributeUse->valueConstraint()) {
                conflictingAttribute = attributeUse->attribute();
                return true;
            }
        }
    }

    return false;
}

bool XsdSchemaChecker::particleEqualsRecursively(const XsdParticle::Ptr &particle, const XsdParticle::Ptr &otherParticle) const
{
    // @see http://www.w3.org/TR/xmlschema11-1/#cos-particle-extend
    //TODO: find out what 'properties' of a particle should be checked here...

    if (particle->minimumOccurs() != otherParticle->minimumOccurs())
        return false;

    if (particle->maximumOccursUnbounded() != otherParticle->maximumOccursUnbounded())
        return false;

    if (particle->maximumOccurs() != otherParticle->maximumOccurs())
        return false;

    const XsdTerm::Ptr term = particle->term();
    const XsdTerm::Ptr otherTerm = otherParticle->term();

    if (term->isElement() && !(otherTerm->isElement()))
        return false;

    if (term->isModelGroup() && !(otherTerm->isModelGroup()))
        return false;

    if (term->isWildcard() && !(otherTerm->isWildcard()))
        return false;

    if (term->isElement()) {
        const XsdElement::Ptr element = term;
        const XsdElement::Ptr otherElement = otherTerm;

        if (element->name(m_namePool) != otherElement->name(m_namePool))
            return false;

        if (element->type()->name(m_namePool) != otherElement->type()->name(m_namePool))
            return false;
    }

    if (term->isModelGroup()) {
        const XsdModelGroup::Ptr group = term;
        const XsdModelGroup::Ptr otherGroup = otherTerm;

        if (group->particles().count() != otherGroup->particles().count())
            return false;

        for (int i = 0; i < group->particles().count(); ++i) {
            if (!particleEqualsRecursively(group->particles().at(i), otherGroup->particles().at(i)))
                return false;
        }
    }

    if (term->isWildcard()) {
    }

    return true;
}

bool XsdSchemaChecker::isValidParticleExtension(const XsdParticle::Ptr &extension, const XsdParticle::Ptr &base) const
{
    // @see http://www.w3.org/TR/xmlschema11-1/#cos-particle-extend

    // 1
    if (extension == base)
        return true;

    // 2
    if (extension->minimumOccurs() == 1 && extension->maximumOccurs() == 1 && extension->maximumOccursUnbounded() == false) {
        if (extension->term()->isModelGroup()) {
            const XsdModelGroup::Ptr modelGroup = extension->term();
            if (modelGroup->compositor() == XsdModelGroup::SequenceCompositor) {
                if (particleEqualsRecursively(modelGroup->particles().first(), base))
                    return true;
            }
        }
    }

    // 3
    if (extension->minimumOccurs() == base->minimumOccurs()) { // 3.1
        if (extension->term()->isModelGroup() && base->term()->isModelGroup()) {
            const XsdModelGroup::Ptr extensionGroup(extension->term());
            const XsdModelGroup::Ptr baseGroup(base->term());

            if (extensionGroup->compositor() == XsdModelGroup::AllCompositor && baseGroup->compositor() == XsdModelGroup::AllCompositor) {
                const XsdParticle::List extensionParticles = extensionGroup->particles();
                const XsdParticle::List baseParticles = baseGroup->particles();
                for (int i = 0; i < baseParticles.count() && i < extensionParticles.count(); ++i) {
                    if (baseParticles.at(i) != extensionParticles.at(i))
                        return false;
                }
            }
        }
    }

    return false;
}

QSet<XsdElement::Ptr> collectAllElements(const XsdParticle::Ptr &particle)
{
    QSet<XsdElement::Ptr> elements;

    const XsdTerm::Ptr term(particle->term());
    if (term->isElement()) {
        elements.insert(XsdElement::Ptr(term));
    } else if (term->isModelGroup()) {
        const XsdModelGroup::Ptr group(term);

        for (int i = 0; i < group->particles().count(); ++i)
            elements.unite(collectAllElements(group->particles().at(i)));
    }

    return elements;
}

QSet<XsdElement::Ptr> collectAllElements(const XsdSchema::Ptr &schema)
{
    QSet<XsdElement::Ptr> elements;

    // collect global elements
    const XsdElement::List elementList = schema->elements();
    for (int i = 0; i < elementList.count(); ++i)
        elements.insert(elementList.at(i));

    // collect all elements from global groups
    const XsdModelGroup::List groupList = schema->elementGroups();
    for (int i = 0; i < groupList.count(); ++i) {
        const XsdModelGroup::Ptr group(groupList.at(i));

        for (int j = 0; j < group->particles().count(); ++j)
            elements.unite(collectAllElements(group->particles().at(j)));
    }

    // collect all elements from complex type definitions
    SchemaType::List types;
    types << schema->types() << schema->anonymousTypes();

    for (int i = 0; i < types.count(); ++i) {
        if (types.at(i)->isComplexType() && types.at(i)->isDefinedBySchema()) {
            const XsdComplexType::Ptr complexType(types.at(i));
            if (complexType->contentType()->particle())
                elements.unite(collectAllElements(complexType->contentType()->particle()));
        }
    }

    return elements;
}

bool XsdSchemaChecker::elementSequenceAccepted(const XsdModelGroup::Ptr &sequence, const XsdParticle::Ptr &particle) const
{
    // @see http://www.w3.org/TR/xmlschema11-1/#cvc-accept

    if (particle->term()->isWildcard()) { // 1
        const XsdWildcard::Ptr wildcard(particle->term());

        // 1.1
        if ((unsigned int)sequence->particles().count() < particle->minimumOccurs())
            return false;

        // 1.2
        if (!particle->maximumOccursUnbounded()) {
            if ((unsigned int)sequence->particles().count() > particle->maximumOccurs())
                return false;
        }

        // 1.3
        const XsdParticle::List particles(sequence->particles());
        for (int i = 0; i < particles.count(); ++i) {
            if (particles.at(i)->term()->isElement()) {
                if (!XsdSchemaHelper::wildcardAllowsExpandedName(XsdElement::Ptr(particles.at(i)->term())->name(m_namePool), wildcard, m_namePool))
                    return false;
            }
        }
    } else if (particle->term()->isElement()) { // 2
        const XsdElement::Ptr element(particle->term());

        // 2.1
        if ((unsigned int)sequence->particles().count() < particle->minimumOccurs())
            return false;

        // 2.2
        if (!particle->maximumOccursUnbounded()) {
            if ((unsigned int)sequence->particles().count() > particle->maximumOccurs())
                return false;
        }

        // 2.3
        const XsdParticle::List particles(sequence->particles());
        for (int i = 0; i < particles.count(); ++i) {
            bool isValid = false;
            Q_UNUSED(isValid)
            if (particles.at(i)->term()->isElement()) {
                const XsdElement::Ptr seqElement(particles.at(i)->term());

                // 2.3.1
                if (element->name(m_namePool) == seqElement->name(m_namePool))
                    isValid = true;

                // 2.3.2
                if (element->scope() && element->scope()->variety() == XsdElement::Scope::Global) {
                    if (!(element->disallowedSubstitutions() & NamedSchemaComponent::SubstitutionConstraint)) {
                        //TODO: continue
                    }
                }
            }
        }
    }

    return true;
}

QT_END_NAMESPACE
