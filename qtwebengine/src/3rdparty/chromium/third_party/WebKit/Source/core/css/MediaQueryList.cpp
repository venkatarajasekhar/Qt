/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "core/css/MediaQueryList.h"

#include "core/css/MediaList.h"
#include "core/css/MediaQueryEvaluator.h"
#include "core/css/MediaQueryListListener.h"
#include "core/css/MediaQueryMatcher.h"

namespace WebCore {

PassRefPtrWillBeRawPtr<MediaQueryList> MediaQueryList::create(PassRefPtrWillBeRawPtr<MediaQueryMatcher> vector, PassRefPtrWillBeRawPtr<MediaQuerySet> media, bool matches)
{
    return adoptRefWillBeNoop(new MediaQueryList(vector, media, matches));
}

MediaQueryList::MediaQueryList(PassRefPtrWillBeRawPtr<MediaQueryMatcher> vector, PassRefPtrWillBeRawPtr<MediaQuerySet> media, bool matches)
    : m_matcher(vector)
    , m_media(media)
    , m_evaluationRound(m_matcher->evaluationRound())
    , m_changeRound(m_evaluationRound - 1) // m_evaluationRound and m_changeRound initial values must be different.
    , m_matches(matches)
{
}

DEFINE_EMPTY_DESTRUCTOR_WILL_BE_REMOVED(MediaQueryList)

String MediaQueryList::media() const
{
    return m_media->mediaText();
}

void MediaQueryList::addListener(PassRefPtrWillBeRawPtr<MediaQueryListListener> listener)
{
    if (!listener)
        return;

    m_matcher->addListener(listener, this);
}

void MediaQueryList::removeListener(PassRefPtrWillBeRawPtr<MediaQueryListListener> listener)
{
    if (!listener)
        return;

    m_matcher->removeListener(listener.get(), this);
}

bool MediaQueryList::evaluate(MediaQueryEvaluator* evaluator)
{
    if (m_evaluationRound != m_matcher->evaluationRound() && evaluator)
        setMatches(evaluator->eval(m_media.get()));
    return m_changeRound == m_matcher->evaluationRound();
}

void MediaQueryList::setMatches(bool newValue)
{
    m_evaluationRound = m_matcher->evaluationRound();

    if (newValue == m_matches)
        return;

    m_matches = newValue;
    m_changeRound = m_evaluationRound;
}

bool MediaQueryList::matches()
{
    if (m_evaluationRound != m_matcher->evaluationRound())
        setMatches(m_matcher->evaluate(m_media.get()));
    return m_matches;
}

void MediaQueryList::trace(Visitor* visitor)
{
    visitor->trace(m_matcher);
    visitor->trace(m_media);
}

}
