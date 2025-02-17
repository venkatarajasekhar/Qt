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

#include "config.h"
#include "web/ContextFeaturesClientImpl.h"

#include "core/dom/Document.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "public/web/WebDocument.h"
#include "public/web/WebPermissionClient.h"
#include "web/WebLocalFrameImpl.h"

using namespace WebCore;

namespace blink {

class ContextFeaturesCache FINAL : public NoBaseWillBeGarbageCollectedFinalized<ContextFeaturesCache>, public DocumentSupplement {
    WILL_BE_USING_GARBAGE_COLLECTED_MIXIN(ContextFeaturesCache);
public:
    class Entry {
    public:
        enum Value {
            IsEnabled,
            IsDisabled,
            NeedsRefresh
        };

        Entry()
            : m_value(NeedsRefresh)
            , m_defaultValue(false)
        { }

        bool isEnabled() const
        {
            ASSERT(m_value != NeedsRefresh);
            return m_value == IsEnabled;
        }

        void set(bool value, bool defaultValue)
        {
            m_value = value ? IsEnabled : IsDisabled;
            m_defaultValue = defaultValue;
        }

        bool needsRefresh(bool defaultValue) const
        {
            return m_value == NeedsRefresh || m_defaultValue != defaultValue;
        }

    private:
        Value m_value;
        bool m_defaultValue; // Needs to be traked as a part of the signature since it can be changed dynamically.
    };

    static const char* supplementName();
    static ContextFeaturesCache& from(Document&);

    Entry& entryFor(ContextFeatures::FeatureType type)
    {
        size_t index = static_cast<size_t>(type);
        ASSERT_WITH_SECURITY_IMPLICATION(index < ContextFeatures::FeatureTypeSize);
        return m_entries[index];
    }

    void validateAgainst(Document*);

private:
    String m_domain;
    Entry m_entries[ContextFeatures::FeatureTypeSize];
};

const char* ContextFeaturesCache::supplementName()
{
    return "ContextFeaturesCache";
}

ContextFeaturesCache& ContextFeaturesCache::from(Document& document)
{
    ContextFeaturesCache* cache = static_cast<ContextFeaturesCache*>(DocumentSupplement::from(document, supplementName()));
    if (!cache) {
        cache = new ContextFeaturesCache();
        DocumentSupplement::provideTo(document, supplementName(), adoptPtrWillBeNoop(cache));
    }

    return *cache;
}

void ContextFeaturesCache::validateAgainst(Document* document)
{
    String currentDomain = document->securityOrigin()->domain();
    if (currentDomain == m_domain)
        return;
    m_domain = currentDomain;
    for (size_t i = 0; i < ContextFeatures::FeatureTypeSize; ++i)
        m_entries[i] = Entry();
}

bool ContextFeaturesClientImpl::isEnabled(Document* document, ContextFeatures::FeatureType type, bool defaultValue)
{
    ASSERT(document);
    ContextFeaturesCache::Entry& cache = ContextFeaturesCache::from(*document).entryFor(type);
    if (cache.needsRefresh(defaultValue))
        cache.set(askIfIsEnabled(document, type, defaultValue), defaultValue);
    return cache.isEnabled();
}

void ContextFeaturesClientImpl::urlDidChange(Document* document)
{
    ASSERT(document);
    ContextFeaturesCache::from(*document).validateAgainst(document);
}

bool ContextFeaturesClientImpl::askIfIsEnabled(Document* document, ContextFeatures::FeatureType type, bool defaultValue)
{
    WebLocalFrameImpl* frame = WebLocalFrameImpl::fromFrame(document->frame());
    if (!frame || !frame->permissionClient())
        return defaultValue;

    switch (type) {
    case ContextFeatures::MutationEvents:
        return frame->permissionClient()->allowMutationEvents(defaultValue);
    case ContextFeatures::PushState:
        return frame->permissionClient()->allowPushState();
    default:
        return defaultValue;
    }
}

} // namespace blink
