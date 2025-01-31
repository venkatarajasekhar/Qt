// Copyright (c) 2005, 2007, Google Inc.
// All rights reserved.
// Copyright (C) 2005, 2006, 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "config.h"
#include "wtf/FastMalloc.h"

#include <string.h>

namespace WTF {

void* fastZeroedMalloc(size_t n)
{
    void* result = fastMalloc(n);
    memset(result, 0, n);
    return result;
}

char* fastStrDup(const char* src)
{
    size_t len = strlen(src) + 1;
    char* dup = static_cast<char*>(fastMalloc(len));
    memcpy(dup, src, len);
    return dup;
}

// TODO: remove these two.
void releaseFastMallocFreeMemory() { }

FastMallocStatistics fastMallocStatistics()
{
    FastMallocStatistics statistics = { 0, 0, 0 };
    return statistics;
}

} // namespace WTF

#if USE(SYSTEM_MALLOC)

#include "wtf/Assertions.h"

#include <stdlib.h>

namespace WTF {

void fastMallocShutdown()
{
}

void* fastMalloc(size_t n)
{
    void* result = malloc(n);
    ASSERT(result);  // We expect tcmalloc underneath, which would crash instead of getting here.

    return result;
}

void fastFree(void* p)
{
    free(p);
}

void* fastRealloc(void* p, size_t n)
{
    void* result = realloc(p, n);
    ASSERT(result);  // We expect tcmalloc underneath, which would crash instead of getting here.

    return result;
}

} // namespace WTF

#else // USE(SYSTEM_MALLOC)

#include "wtf/PartitionAlloc.h"
#include "wtf/SpinLock.h"

namespace WTF {

static PartitionAllocatorGeneric gPartition;
static int gLock = 0;
static bool gInitialized = false;

void fastMallocShutdown()
{
    gPartition.shutdown();
}

void* fastMalloc(size_t n)
{
    if (UNLIKELY(!gInitialized)) {
        spinLockLock(&gLock);
        if (!gInitialized) {
            gInitialized = true;
            gPartition.init();
        }
        spinLockUnlock(&gLock);
    }
    return partitionAllocGeneric(gPartition.root(), n);
}

void fastFree(void* p)
{
    partitionFreeGeneric(gPartition.root(), p);
}

void* fastRealloc(void* p, size_t n)
{
    return partitionReallocGeneric(gPartition.root(), p, n);
}

} // namespace WTF

#endif // USE(SYSTEM_MALLOC)
