/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef StyleGridItemData_h
#define StyleGridItemData_h


#include "GridPosition.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class StyleGridItemData : public RefCounted<StyleGridItemData> {
public:
    static PassRefPtr<StyleGridItemData> create() { return adoptRef(new StyleGridItemData); }
    PassRefPtr<StyleGridItemData> copy() const { return adoptRef(new StyleGridItemData(*this)); }

    bool operator==(const StyleGridItemData& o) const
    {
        return m_gridStart == o.m_gridStart && m_gridEnd == o.m_gridEnd
            && m_gridBefore == o.m_gridBefore && m_gridAfter == o.m_gridAfter;
    }

    bool operator!=(const StyleGridItemData& o) const
    {
        return !(*this == o);
    }

    GridPosition m_gridStart;
    GridPosition m_gridEnd;
    GridPosition m_gridBefore;
    GridPosition m_gridAfter;

private:
    StyleGridItemData();
    StyleGridItemData(const StyleGridItemData&);
};

} // namespace WebCore

#endif // StyleGridItemData_h
