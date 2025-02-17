/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef NavigatorStorageQuota_h
#define NavigatorStorageQuota_h

#if ENABLE(QUOTA)

#include "DOMWindowProperty.h"
#include "Supplementable.h"

namespace WebCore {

class Frame;
class StorageQuota;
class Navigator;

class NavigatorStorageQuota : public Supplement<Navigator>, public DOMWindowProperty {
public:
    virtual ~NavigatorStorageQuota();
    static NavigatorStorageQuota* from(Navigator*);

    static StorageQuota* webkitTemporaryStorage(Navigator*);
    static StorageQuota* webkitPersistentStorage(Navigator*);
    StorageQuota* webkitTemporaryStorage() const;
    StorageQuota* webkitPersistentStorage() const;

private:
    explicit NavigatorStorageQuota(Frame*);
    static const char* supplementName();

    mutable RefPtr<StorageQuota> m_temporaryStorage;
    mutable RefPtr<StorageQuota> m_persistentStorage;
};

} // namespace WebCore

#endif // ENABLE(QUOTA)

#endif // NavigatorStorageQuota_h
