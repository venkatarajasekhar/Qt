/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef MHTMLArchive_h
#define MHTMLArchive_h

#include "platform/mhtml/ArchiveResource.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/RefPtr.h"
#include "wtf/Vector.h"

namespace WebCore {

class KURL;
class MHTMLParser;
class SharedBuffer;

struct SerializedResource;

class PLATFORM_EXPORT MHTMLArchive FINAL : public RefCounted<MHTMLArchive> {
public:
    static PassRefPtr<MHTMLArchive> create();
    static PassRefPtr<MHTMLArchive> create(const KURL&, SharedBuffer*);

    enum EncodingPolicy {
        UseDefaultEncoding,
        UseBinaryEncoding
    };

    // Binary encoding results in smaller MHTML files but they might not work in other browsers.
    static PassRefPtr<SharedBuffer> generateMHTMLData(const Vector<SerializedResource>&, EncodingPolicy, const String& title, const String& mimeType);

    ~MHTMLArchive();
    ArchiveResource* mainResource() { return m_mainResource.get(); }
    const Vector<RefPtr<ArchiveResource> >& subresources() const { return m_subresources; }
    const Vector<RefPtr<MHTMLArchive> >& subframeArchives() const { return m_subframeArchives; }

private:
    friend class MHTMLParser;
    MHTMLArchive();

    void setMainResource(PassRefPtr<ArchiveResource> mainResource) { m_mainResource = mainResource; }
    void addSubresource(PassRefPtr<ArchiveResource> subResource) { m_subresources.append(subResource); }
    void addSubframeArchive(PassRefPtr<MHTMLArchive> subframeArchive) { m_subframeArchives.append(subframeArchive); }

    void clearAllSubframeArchives();
    void clearAllSubframeArchivesImpl(Vector<RefPtr<MHTMLArchive> >* clearedArchives);

    RefPtr<ArchiveResource> m_mainResource;
    Vector<RefPtr<ArchiveResource> > m_subresources;
    Vector<RefPtr<MHTMLArchive> > m_subframeArchives;
};

}

#endif
