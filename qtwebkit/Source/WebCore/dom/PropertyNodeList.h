/*
 * Copyright (c) 2012 Motorola Mobility, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of Motorola Mobility, Inc. nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PropertyNodeList_h
#define PropertyNodeList_h

#if ENABLE(MICRODATA)
#include "LiveNodeList.h"
#include "MicroDataItemValue.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

typedef Vector<RefPtr<MicroDataItemValue> > PropertyValueArray;

class PropertyNodeList : public LiveNodeList {
public:
    static PassRefPtr<PropertyNodeList> create(Node* rootNode, const String& name)
    {
        return adoptRef(new PropertyNodeList(rootNode, name));
    }

    ~PropertyNodeList();

    PropertyValueArray getValues() const;
    void updateRefElements() const;

private:
    explicit PropertyNodeList(Node* rootNode, const String& name);

    virtual bool nodeMatches(Element*) const OVERRIDE;

    bool elementIsPropertyOfRefElement(const Node*, const Node*) const;

    String m_name;
    mutable Vector<HTMLElement*> m_itemRefElementsCache;
};

} // namespace WebCore

#endif // ENABLE(MICRODATA)

#endif // StaticNodeList_h
