/*
 * Copyright (C) 2008 Nikolas Zimmermann <zimmermann@kde.org>
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
 */

#ifndef SVGFontData_h
#define SVGFontData_h

#if ENABLE(SVG_FONTS)
#include "platform/fonts/CustomFontData.h"
#include "wtf/WeakPtr.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class SimpleFontData;
class SVGFontElement;
class SVGFontFaceElement;

class SVGFontData FINAL : public CustomFontData {
public:
    static PassRefPtr<SVGFontData> create(SVGFontFaceElement* element)
    {
        return adoptRef(new SVGFontData(element));
    }

    virtual ~SVGFontData();

    virtual bool isSVGFont() const OVERRIDE { return true; };
    virtual bool shouldSkipDrawing() const OVERRIDE;
    virtual void initializeFontData(SimpleFontData*, float fontSize) OVERRIDE;
    virtual float widthForSVGGlyph(Glyph, float fontSize) const OVERRIDE;
    virtual bool fillSVGGlyphPage(GlyphPage*, unsigned offset, unsigned length, UChar* buffer, unsigned bufferLength, const SimpleFontData*) const OVERRIDE;
    virtual bool applySVGGlyphSelection(WidthIterator&, GlyphData&, bool mirror, int currentCharacter, unsigned& advanceLength) const OVERRIDE;

    SVGFontFaceElement* svgFontFaceElement() const;

    float horizontalOriginX() const { return m_horizontalOriginX; }
    float horizontalOriginY() const { return m_horizontalOriginY; }
    float horizontalAdvanceX() const { return m_horizontalAdvanceX; }

    float verticalOriginX() const { return m_verticalOriginX; }
    float verticalOriginY() const { return m_verticalOriginY; }
    float verticalAdvanceY() const { return m_verticalAdvanceY; }

private:
    SVGFontData(SVGFontFaceElement*);

    bool fillBMPGlyphs(SVGFontElement*, GlyphPage* , unsigned offset, unsigned length, UChar* buffer, const SimpleFontData*) const;
    bool fillNonBMPGlyphs(SVGFontElement*, GlyphPage* , unsigned offset, unsigned length, UChar* buffer, const SimpleFontData*) const;

    String createStringWithMirroredCharacters(const String&) const;

    // Ths SVGFontFaceElement is kept alive --
    // 1) in the external font case: by the CSSFontFaceSource, which holds a reference to the external SVG document
    //    containing the element;
    // 2) in the in-document font case: by virtue of being in the document tree and making sure that when it is removed
    //    from the document, it removes the @font-face rule it owns from the document's mapped element sheet and forces
    //    a style update.
    WeakPtr<SVGFontFaceElement> m_svgFontFaceElement;

    float m_horizontalOriginX;
    float m_horizontalOriginY;
    float m_horizontalAdvanceX;

    float m_verticalOriginX;
    float m_verticalOriginY;
    float m_verticalAdvanceY;
};

} // namespace WebCore

#endif
#endif // SVGFontData_h
