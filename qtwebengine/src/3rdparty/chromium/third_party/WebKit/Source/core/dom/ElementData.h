/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ElementData_h
#define ElementData_h

#include "core/dom/Attribute.h"
#include "core/dom/SpaceSplitString.h"
#include "wtf/text/AtomicString.h"

namespace WebCore {

class Attr;
class ShareableElementData;
class StylePropertySet;
class UniqueElementData;

class AttributeCollection {
public:
    typedef const Attribute* const_iterator;

    AttributeCollection(const Attribute* array, unsigned size)
        : m_array(array)
        , m_size(size)
    { }

    const_iterator begin() const { return m_array; }
    const_iterator end() const { return m_array + m_size; }

    unsigned size() const { return m_size; }

private:
    const Attribute* m_array;
    unsigned m_size;
};

// ElementData represents very common, but not necessarily unique to an element,
// data such as attributes, inline style, and parsed class names and ids.
class ElementData : public RefCounted<ElementData> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    // Override RefCounted's deref() to ensure operator delete is called on
    // the appropriate subclass type.
    void deref();

    void clearClass() const { m_classNames.clear(); }
    void setClass(const AtomicString& className, bool shouldFoldCase) const { m_classNames.set(className, shouldFoldCase); }
    const SpaceSplitString& classNames() const { return m_classNames; }

    const AtomicString& idForStyleResolution() const { return m_idForStyleResolution; }
    void setIdForStyleResolution(const AtomicString& newId) const { m_idForStyleResolution = newId; }

    const StylePropertySet* inlineStyle() const { return m_inlineStyle.get(); }

    const StylePropertySet* presentationAttributeStyle() const;

    // This is not a trivial getter and its return value should be cached for performance.
    size_t attributeCount() const;
    bool hasAttributes() const { return !!attributeCount(); }

    AttributeCollection attributes() const;

    const Attribute& attributeAt(unsigned index) const;
    const Attribute* findAttributeByName(const QualifiedName&) const;
    size_t findAttributeIndexByName(const QualifiedName&, bool shouldIgnoreCase = false) const;
    size_t findAttributeIndexByName(const AtomicString& name, bool shouldIgnoreAttributeCase) const;
    size_t findAttrNodeIndex(Attr*) const;

    bool hasID() const { return !m_idForStyleResolution.isNull(); }
    bool hasClass() const { return !m_classNames.isNull(); }

    bool isEquivalent(const ElementData* other) const;

    bool isUnique() const { return m_isUnique; }

protected:
    ElementData();
    explicit ElementData(unsigned arraySize);
    ElementData(const ElementData&, bool isUnique);

    // Keep the type in a bitfield instead of using virtual destructors to avoid adding a vtable.
    unsigned m_isUnique : 1;
    unsigned m_arraySize : 28;
    mutable unsigned m_presentationAttributeStyleIsDirty : 1;
    mutable unsigned m_styleAttributeIsDirty : 1;
    mutable unsigned m_animatedSVGAttributesAreDirty : 1;

    mutable RefPtr<StylePropertySet> m_inlineStyle;
    mutable SpaceSplitString m_classNames;
    mutable AtomicString m_idForStyleResolution;

private:
    friend class Element;
    friend class ShareableElementData;
    friend class UniqueElementData;
    friend class SVGElement;

    void destroy();

    const Attribute* attributeBase() const;
    const Attribute* findAttributeByName(const AtomicString& name, bool shouldIgnoreAttributeCase) const;
    size_t findAttributeIndexByNameSlowCase(const AtomicString& name, bool shouldIgnoreAttributeCase) const;

    PassRefPtr<UniqueElementData> makeUniqueCopy() const;
};

#if COMPILER(MSVC)
#pragma warning(push)
#pragma warning(disable: 4200) // Disable "zero-sized array in struct/union" warning
#endif

// SharableElementData is managed by ElementDataCache and is produced by
// the parser during page load for elements that have identical attributes. This
// is a memory optimization since it's very common for many elements to have
// duplicate sets of attributes (ex. the same classes).
class ShareableElementData FINAL : public ElementData {
public:
    static PassRefPtr<ShareableElementData> createWithAttributes(const Vector<Attribute>&);

    explicit ShareableElementData(const Vector<Attribute>&);
    explicit ShareableElementData(const UniqueElementData&);
    ~ShareableElementData();

    Attribute m_attributeArray[0];
};

#if COMPILER(MSVC)
#pragma warning(pop)
#endif

// UniqueElementData is created when an element needs to mutate its attributes
// or gains presentation attribute style (ex. width="10"). It does not need to
// be created to fill in values in the ElementData that are derived from
// attributes. For example populating the m_inlineStyle from the style attribute
// doesn't require a UniqueElementData as all elements with the same style
// attribute will have the same inline style.
class UniqueElementData FINAL : public ElementData {
public:
    static PassRefPtr<UniqueElementData> create();
    PassRefPtr<ShareableElementData> makeShareableCopy() const;

    // These functions do no error/duplicate checking.
    void appendAttribute(const QualifiedName&, const AtomicString&);
    void removeAttributeAt(size_t index);

    Attribute& attributeAt(unsigned index);
    Attribute* findAttributeByName(const QualifiedName&);

    UniqueElementData();
    explicit UniqueElementData(const ShareableElementData&);
    explicit UniqueElementData(const UniqueElementData&);

    // FIXME: We might want to support sharing element data for elements with
    // presentation attribute style. Lots of table cells likely have the same
    // attributes. Most modern pages don't use presentation attributes though
    // so this might not make sense.
    mutable RefPtr<StylePropertySet> m_presentationAttributeStyle;
    Vector<Attribute, 4> m_attributeVector;
};

inline void ElementData::deref()
{
    if (!derefBase())
        return;
    destroy();
}

inline size_t ElementData::attributeCount() const
{
    if (isUnique())
        return static_cast<const UniqueElementData*>(this)->m_attributeVector.size();
    return m_arraySize;
}

inline const StylePropertySet* ElementData::presentationAttributeStyle() const
{
    if (!m_isUnique)
        return 0;
    return static_cast<const UniqueElementData*>(this)->m_presentationAttributeStyle.get();
}

inline const Attribute* ElementData::findAttributeByName(const AtomicString& name, bool shouldIgnoreAttributeCase) const
{
    size_t index = findAttributeIndexByName(name, shouldIgnoreAttributeCase);
    if (index != kNotFound)
        return &attributeAt(index);
    return 0;
}

inline const Attribute* ElementData::attributeBase() const
{
    if (m_isUnique)
        return static_cast<const UniqueElementData*>(this)->m_attributeVector.begin();
    return static_cast<const ShareableElementData*>(this)->m_attributeArray;
}

inline size_t ElementData::findAttributeIndexByName(const QualifiedName& name, bool shouldIgnoreCase) const
{
    AttributeCollection attributes = this->attributes();
    AttributeCollection::const_iterator end = attributes.end();
    unsigned index = 0;
    for (AttributeCollection::const_iterator it = attributes.begin(); it != end; ++it, ++index) {
        if (it->name().matchesPossiblyIgnoringCase(name, shouldIgnoreCase))
            return index;
    }
    return kNotFound;
}

// We use a boolean parameter instead of calling shouldIgnoreAttributeCase so that the caller
// can tune the behavior (hasAttribute is case sensitive whereas getAttribute is not).
inline size_t ElementData::findAttributeIndexByName(const AtomicString& name, bool shouldIgnoreAttributeCase) const
{
    bool doSlowCheck = shouldIgnoreAttributeCase;

    // Optimize for the case where the attribute exists and its name exactly matches.
    AttributeCollection attributes = this->attributes();
    AttributeCollection::const_iterator end = attributes.end();
    unsigned index = 0;
    for (AttributeCollection::const_iterator it = attributes.begin(); it != end; ++it, ++index) {
        // FIXME: Why check the prefix? Namespaces should be all that matter.
        // Most attributes (all of HTML and CSS) have no namespace.
        if (!it->name().hasPrefix()) {
            if (name == it->localName())
                return index;
        } else {
            doSlowCheck = true;
        }
    }

    if (doSlowCheck)
        return findAttributeIndexByNameSlowCase(name, shouldIgnoreAttributeCase);
    return kNotFound;
}

inline AttributeCollection ElementData::attributes() const
{
    if (isUnique()) {
        const Vector<Attribute, 4>& attributeVector = static_cast<const UniqueElementData*>(this)->m_attributeVector;
        return AttributeCollection(attributeVector.data(), attributeVector.size());
    }
    return AttributeCollection(static_cast<const ShareableElementData*>(this)->m_attributeArray, m_arraySize);
}

inline const Attribute* ElementData::findAttributeByName(const QualifiedName& name) const
{
    AttributeCollection attributes = this->attributes();
    AttributeCollection::const_iterator end = attributes.end();
    for (AttributeCollection::const_iterator it = attributes.begin(); it != end; ++it) {
        if (it->name().matches(name))
            return it;
    }
    return 0;
}

inline const Attribute& ElementData::attributeAt(unsigned index) const
{
    RELEASE_ASSERT(index < attributeCount());
    ASSERT(attributeBase() + index);
    return *(attributeBase() + index);
}

inline void UniqueElementData::appendAttribute(const QualifiedName& attributeName, const AtomicString& value)
{
    m_attributeVector.append(Attribute(attributeName, value));
}

inline void UniqueElementData::removeAttributeAt(size_t index)
{
    m_attributeVector.remove(index);
}

inline Attribute& UniqueElementData::attributeAt(unsigned index)
{
    return m_attributeVector.at(index);
}

} // namespace WebCore

#endif // ElementData_h
