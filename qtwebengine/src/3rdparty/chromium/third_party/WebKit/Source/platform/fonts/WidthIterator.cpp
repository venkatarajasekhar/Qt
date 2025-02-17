/*
 * Copyright (C) 2003, 2006, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Holger Hans Peter Freyther
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "platform/fonts/WidthIterator.h"

#include "platform/fonts/Character.h"
#include "platform/fonts/FontPlatformFeatures.h"
#include "platform/fonts/GlyphBuffer.h"
#include "platform/fonts/Latin1TextIterator.h"
#include "platform/fonts/SimpleFontData.h"
#include "platform/text/SurrogatePairAwareTextIterator.h"
#include "wtf/MathExtras.h"

using namespace WTF;
using namespace Unicode;
using namespace std;

namespace WebCore {

WidthIterator::WidthIterator(const Font* font, const TextRun& run, HashSet<const SimpleFontData*>* fallbackFonts, bool accountForGlyphBounds, bool forTextEmphasis)
    : m_font(font)
    , m_run(run)
    , m_currentCharacter(0)
    , m_runWidthSoFar(0)
    , m_isAfterExpansion(!run.allowsLeadingExpansion())
    , m_finalRoundingWidth(0)
    , m_typesettingFeatures(font->fontDescription().typesettingFeatures())
    , m_fallbackFonts(fallbackFonts)
    , m_accountForGlyphBounds(accountForGlyphBounds)
    , m_maxGlyphBoundingBoxY(numeric_limits<float>::min())
    , m_minGlyphBoundingBoxY(numeric_limits<float>::max())
    , m_firstGlyphOverflow(0)
    , m_lastGlyphOverflow(0)
    , m_forTextEmphasis(forTextEmphasis)
{
    // If the padding is non-zero, count the number of spaces in the run
    // and divide that by the padding for per space addition.
    m_expansion = m_run.expansion();
    if (!m_expansion)
        m_expansionPerOpportunity = 0;
    else {
        bool isAfterExpansion = m_isAfterExpansion;
        unsigned expansionOpportunityCount = m_run.is8Bit() ? Character::expansionOpportunityCount(m_run.characters8(), m_run.length(), m_run.direction(), isAfterExpansion) : Character::expansionOpportunityCount(m_run.characters16(), m_run.length(), m_run.direction(), isAfterExpansion);
        if (isAfterExpansion && !m_run.allowsTrailingExpansion())
            expansionOpportunityCount--;

        if (!expansionOpportunityCount)
            m_expansionPerOpportunity = 0;
        else
            m_expansionPerOpportunity = m_expansion / expansionOpportunityCount;
    }
}

GlyphData WidthIterator::glyphDataForCharacter(UChar32 character, bool mirror, int currentCharacter, unsigned& advanceLength)
{
    ASSERT(m_font);

#if ENABLE(SVG_FONTS)
    if (TextRun::RenderingContext* renderingContext = m_run.renderingContext())
        return renderingContext->glyphDataForCharacter(*m_font, m_run, *this, character, mirror, currentCharacter, advanceLength);
#endif

    return m_font->glyphDataForCharacter(character, mirror);
}

struct OriginalAdvancesForCharacterTreatedAsSpace {
public:
    OriginalAdvancesForCharacterTreatedAsSpace(bool isSpace, float advanceBefore, float advanceAt)
        : characterIsSpace(isSpace)
        , advanceBeforeCharacter(advanceBefore)
        , advanceAtCharacter(advanceAt)
    {
    }

    bool characterIsSpace;
    float advanceBeforeCharacter;
    float advanceAtCharacter;
};

typedef Vector<pair<int, OriginalAdvancesForCharacterTreatedAsSpace>, 64> CharactersTreatedAsSpace;

static inline float applyFontTransforms(GlyphBuffer* glyphBuffer, unsigned& lastGlyphCount, TypesettingFeatures typesettingFeatures, CharactersTreatedAsSpace& charactersTreatedAsSpace)
{
    ASSERT(typesettingFeatures & (Kerning | Ligatures));

    if (!glyphBuffer)
        return 0;

    unsigned glyphBufferSize = glyphBuffer->size();
    if (glyphBuffer->size() <= lastGlyphCount + 1)
        return 0;

    FloatSize* advances = glyphBuffer->advances(0);
    float widthDifference = 0;
    for (unsigned i = lastGlyphCount; i < glyphBufferSize; ++i)
        widthDifference -= advances[i].width();

    for (size_t i = 0; i < charactersTreatedAsSpace.size(); ++i) {
        int spaceOffset = charactersTreatedAsSpace[i].first;
        const OriginalAdvancesForCharacterTreatedAsSpace& originalAdvances = charactersTreatedAsSpace[i].second;
        if (spaceOffset && !originalAdvances.characterIsSpace)
            glyphBuffer->advances(spaceOffset - 1)->setWidth(originalAdvances.advanceBeforeCharacter);
        glyphBuffer->advances(spaceOffset)->setWidth(originalAdvances.advanceAtCharacter);
    }
    charactersTreatedAsSpace.clear();

    for (unsigned i = lastGlyphCount; i < glyphBufferSize; ++i)
        widthDifference += advances[i].width();

    lastGlyphCount = glyphBufferSize;
    return widthDifference;
}

template <typename TextIterator>
inline unsigned WidthIterator::advanceInternal(TextIterator& textIterator, GlyphBuffer* glyphBuffer)
{
    bool rtl = m_run.rtl();
    bool hasExtraSpacing = (m_font->fontDescription().letterSpacing() || m_font->fontDescription().wordSpacing() || m_expansion) && !m_run.spacingDisabled();

    float widthSinceLastRounding = m_runWidthSoFar;
    m_runWidthSoFar = floorf(m_runWidthSoFar);
    widthSinceLastRounding -= m_runWidthSoFar;

    float lastRoundingWidth = m_finalRoundingWidth;
    FloatRect bounds;

    const SimpleFontData* primaryFont = m_font->primaryFont();
    const SimpleFontData* lastFontData = primaryFont;
    unsigned lastGlyphCount = glyphBuffer ? glyphBuffer->size() : 0;

    UChar32 character = 0;
    unsigned clusterLength = 0;
    CharactersTreatedAsSpace charactersTreatedAsSpace;
    while (textIterator.consume(character, clusterLength)) {
        unsigned advanceLength = clusterLength;
        const GlyphData& glyphData = glyphDataForCharacter(character, rtl, textIterator.currentCharacter(), advanceLength);
        Glyph glyph = glyphData.glyph;
        const SimpleFontData* fontData = glyphData.fontData;

        ASSERT(fontData);

        // Now that we have a glyph and font data, get its width.
        float width;
        if (character == '\t' && m_run.allowTabs())
            width = m_font->tabWidth(*fontData, m_run.tabSize(), m_run.xPos() + m_runWidthSoFar + widthSinceLastRounding);
        else {
            width = fontData->widthForGlyph(glyph);

            // SVG uses horizontalGlyphStretch(), when textLength is used to stretch/squeeze text.
            width *= m_run.horizontalGlyphStretch();

            // We special case spaces in two ways when applying word rounding.
            // First, we round spaces to an adjusted width in all fonts.
            // Second, in fixed-pitch fonts we ensure that all characters that
            // match the width of the space character have the same width as the space character.
            if (m_run.applyWordRounding() && width == fontData->spaceWidth() && (fontData->pitch() == FixedPitch || glyph == fontData->spaceGlyph()))
                width = fontData->adjustedSpaceWidth();
        }

        if (fontData != lastFontData && width) {
            if (shouldApplyFontTransforms())
                m_runWidthSoFar += applyFontTransforms(glyphBuffer, lastGlyphCount, m_typesettingFeatures, charactersTreatedAsSpace);

            lastFontData = fontData;
            if (m_fallbackFonts && fontData != primaryFont) {
                // FIXME: This does a little extra work that could be avoided if
                // glyphDataForCharacter() returned whether it chose to use a small caps font.
                if (m_font->fontDescription().variant() == FontVariantNormal || character == toUpper(character))
                    m_fallbackFonts->add(fontData);
                else {
                    ASSERT(m_font->fontDescription().variant() == FontVariantSmallCaps);
                    const GlyphData& uppercaseGlyphData = m_font->glyphDataForCharacter(toUpper(character), rtl);
                    if (uppercaseGlyphData.fontData != primaryFont)
                        m_fallbackFonts->add(uppercaseGlyphData.fontData);
                }
            }
        }

        if (hasExtraSpacing) {
            // Account for letter-spacing.
            if (width && m_font->fontDescription().letterSpacing())
                width += m_font->fontDescription().letterSpacing();

            static bool expandAroundIdeographs = FontPlatformFeatures::canExpandAroundIdeographsInComplexText();
            bool treatAsSpace = Character::treatAsSpace(character);
            if (treatAsSpace || (expandAroundIdeographs && Character::isCJKIdeographOrSymbol(character))) {
                // Distribute the run's total expansion evenly over all expansion opportunities in the run.
                if (m_expansion) {
                    float previousExpansion = m_expansion;
                    if (!treatAsSpace && !m_isAfterExpansion) {
                        // Take the expansion opportunity before this ideograph.
                        m_expansion -= m_expansionPerOpportunity;
                        float expansionAtThisOpportunity = !m_run.applyWordRounding() ? m_expansionPerOpportunity : roundf(previousExpansion) - roundf(m_expansion);
                        m_runWidthSoFar += expansionAtThisOpportunity;
                        if (glyphBuffer) {
                            if (glyphBuffer->isEmpty()) {
                                if (m_forTextEmphasis)
                                    glyphBuffer->add(fontData->zeroWidthSpaceGlyph(), fontData, m_expansionPerOpportunity);
                                else
                                    glyphBuffer->add(fontData->spaceGlyph(), fontData, expansionAtThisOpportunity);
                            } else
                                glyphBuffer->expandLastAdvance(expansionAtThisOpportunity);
                        }
                        previousExpansion = m_expansion;
                    }
                    if (m_run.allowsTrailingExpansion() || (m_run.ltr() && textIterator.currentCharacter() + advanceLength < static_cast<size_t>(m_run.length()))
                        || (m_run.rtl() && textIterator.currentCharacter())) {
                        m_expansion -= m_expansionPerOpportunity;
                        width += !m_run.applyWordRounding() ? m_expansionPerOpportunity : roundf(previousExpansion) - roundf(m_expansion);
                        m_isAfterExpansion = true;
                    }
                } else
                    m_isAfterExpansion = false;

                // Account for word spacing.
                // We apply additional space between "words" by adding width to the space character.
                if (treatAsSpace && (character != '\t' || !m_run.allowTabs()) && (textIterator.currentCharacter() || character == noBreakSpace) && m_font->fontDescription().wordSpacing())
                    width += m_font->fontDescription().wordSpacing();
            } else
                m_isAfterExpansion = false;
        }

        if (shouldApplyFontTransforms() && glyphBuffer && Character::treatAsSpace(character)) {
            charactersTreatedAsSpace.append(make_pair(glyphBuffer->size(),
                OriginalAdvancesForCharacterTreatedAsSpace(character == ' ',
                    glyphBuffer->size() ? glyphBuffer->advanceAt(glyphBuffer->size() - 1).width() : 0,
                    width)));
        }

        if (m_accountForGlyphBounds) {
            bounds = fontData->boundsForGlyph(glyph);
            if (!textIterator.currentCharacter())
                m_firstGlyphOverflow = max<float>(0, -bounds.x());
        }

        if (m_forTextEmphasis && !Character::canReceiveTextEmphasis(character))
            glyph = 0;

        // Advance past the character we just dealt with.
        textIterator.advance(advanceLength);

        float oldWidth = width;

        // Force characters that are used to determine word boundaries for the rounding hack
        // to be integer width, so following words will start on an integer boundary.
        if (m_run.applyWordRounding() && Character::isRoundingHackCharacter(character)) {
            width = ceilf(width);

            // Since widthSinceLastRounding can lose precision if we include measurements for
            // preceding whitespace, we bypass it here.
            m_runWidthSoFar += width;

            // Since this is a rounding hack character, we should have reset this sum on the previous
            // iteration.
            ASSERT(!widthSinceLastRounding);
        } else {
            // Check to see if the next character is a "rounding hack character", if so, adjust
            // width so that the total run width will be on an integer boundary.
            if ((m_run.applyWordRounding() && textIterator.currentCharacter() < m_run.length() && Character::isRoundingHackCharacter(*(textIterator.characters())))
                || (m_run.applyRunRounding() && textIterator.currentCharacter() >= m_run.length())) {
                float totalWidth = widthSinceLastRounding + width;
                widthSinceLastRounding = ceilf(totalWidth);
                width += widthSinceLastRounding - totalWidth;
                m_runWidthSoFar += widthSinceLastRounding;
                widthSinceLastRounding = 0;
            } else
                widthSinceLastRounding += width;
        }

        if (glyphBuffer)
            glyphBuffer->add(glyph, fontData, (rtl ? oldWidth + lastRoundingWidth : width));

        lastRoundingWidth = width - oldWidth;

        if (m_accountForGlyphBounds) {
            m_maxGlyphBoundingBoxY = max(m_maxGlyphBoundingBoxY, bounds.maxY());
            m_minGlyphBoundingBoxY = min(m_minGlyphBoundingBoxY, bounds.y());
            m_lastGlyphOverflow = max<float>(0, bounds.maxX() - width);
        }
    }

    if (shouldApplyFontTransforms())
        m_runWidthSoFar += applyFontTransforms(glyphBuffer, lastGlyphCount, m_typesettingFeatures, charactersTreatedAsSpace);

    unsigned consumedCharacters = textIterator.currentCharacter() - m_currentCharacter;
    m_currentCharacter = textIterator.currentCharacter();
    m_runWidthSoFar += widthSinceLastRounding;
    m_finalRoundingWidth = lastRoundingWidth;
    return consumedCharacters;
}

unsigned WidthIterator::advance(int offset, GlyphBuffer* glyphBuffer)
{
    int length = m_run.length();

    if (offset > length)
        offset = length;

    if (m_currentCharacter >= static_cast<unsigned>(offset))
        return 0;

    if (m_run.is8Bit()) {
        Latin1TextIterator textIterator(m_run.data8(m_currentCharacter), m_currentCharacter, offset, length);
        return advanceInternal(textIterator, glyphBuffer);
    }

    SurrogatePairAwareTextIterator textIterator(m_run.data16(m_currentCharacter), m_currentCharacter, offset, length);
    return advanceInternal(textIterator, glyphBuffer);
}

bool WidthIterator::advanceOneCharacter(float& width, GlyphBuffer& glyphBuffer)
{
    unsigned oldSize = glyphBuffer.size();
    advance(m_currentCharacter + 1, &glyphBuffer);
    float w = 0;
    for (unsigned i = oldSize; i < glyphBuffer.size(); ++i)
        w += glyphBuffer.advanceAt(i).width();
    width = w;
    return glyphBuffer.size() > oldSize;
}

}
