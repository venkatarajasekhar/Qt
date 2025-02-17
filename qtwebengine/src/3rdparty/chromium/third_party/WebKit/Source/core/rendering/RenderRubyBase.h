/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef RenderRubyBase_h
#define RenderRubyBase_h

#include "core/rendering/RenderBlockFlow.h"

namespace WebCore {

class RenderRubyRun;

class RenderRubyBase FINAL : public RenderBlockFlow {
public:
    virtual ~RenderRubyBase();

    static RenderRubyBase* createAnonymous(Document*);

    virtual const char* renderName() const OVERRIDE { return "RenderRubyBase (anonymous)"; }

    virtual bool isRubyBase() const OVERRIDE { return true; }

    virtual bool isChildAllowed(RenderObject*, RenderStyle*) const OVERRIDE;

private:
    RenderRubyBase();

    virtual ETextAlign textAlignmentForLine(bool endsWithSoftBreak) const OVERRIDE;
    virtual void adjustInlineDirectionLineBounds(unsigned expansionOpportunityCount, float& logicalLeft, float& logicalWidth) const OVERRIDE;

    void moveChildren(RenderRubyBase* toBase, RenderObject* beforeChild = 0);
    void moveInlineChildren(RenderRubyBase* toBase, RenderObject* beforeChild = 0);
    void moveBlockChildren(RenderRubyBase* toBase, RenderObject* beforeChild = 0);

    // Allow RenderRubyRun to manipulate the children within ruby bases.
    friend class RenderRubyRun;
};

} // namespace WebCore

#endif // RenderRubyBase_h
