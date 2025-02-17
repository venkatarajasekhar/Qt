/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
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

#include "config.h"
#include "platform/heap/Visitor.h"

#include "platform/heap/Handle.h"
#include "platform/heap/Heap.h"

namespace WebCore {

#ifndef NDEBUG
void Visitor::checkGCInfo(const void* payload, const GCInfo* gcInfo)
{
    FinalizedHeapObjectHeader::fromPayload(payload)->checkHeader();
#if !defined(COMPONENT_BUILD)
    // On component builds we cannot compare the gcInfos as they are statically
    // defined in each of the components and hence will not match.
    ASSERT(FinalizedHeapObjectHeader::fromPayload(payload)->gcInfo() == gcInfo);
#endif
}

#define DEFINE_VISITOR_CHECK_MARKER(Type)                                    \
    void Visitor::checkGCInfo(const Type* payload, const GCInfo* gcInfo)     \
    {                                                                        \
        HeapObjectHeader::fromPayload(payload)->checkHeader();               \
        Type* object = const_cast<Type*>(payload);                           \
        Address addr = pageHeaderAddress(reinterpret_cast<Address>(object)); \
        BaseHeapPage* page = reinterpret_cast<BaseHeapPage*>(addr);          \
        ASSERT(page->gcInfo() == gcInfo);                                    \
    }

FOR_EACH_TYPED_HEAP(DEFINE_VISITOR_CHECK_MARKER)
#undef DEFINE_VISITOR_CHECK_MARKER
#endif

}
