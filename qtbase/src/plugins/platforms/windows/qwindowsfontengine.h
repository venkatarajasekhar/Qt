/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSFONTENGINE_H
#define QWINDOWSFONTENGINE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qfontengine_p.h>

#include <QtGui/QImage>
#include <QtCore/QSharedPointer>
#include <QtCore/QMetaType>

#include "qtwindows_additional.h"

QT_BEGIN_NAMESPACE

class QWindowsNativeImage;
class QWindowsFontEngineData;

class QWindowsFontEngine : public QFontEngine
{
    friend class QWindowsMultiFontEngine;

public:
    QWindowsFontEngine(const QString &name, HFONT, bool, LOGFONT,
                       const QSharedPointer<QWindowsFontEngineData> &fontEngineData);

    ~QWindowsFontEngine();
    void initFontInfo(const QFontDef &request,
                      HDC fontHdc,  int dpi);

    QFixed lineThickness() const Q_DECL_OVERRIDE;
    Properties properties() const Q_DECL_OVERRIDE;
    void getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics) Q_DECL_OVERRIDE;
    FaceId faceId() const Q_DECL_OVERRIDE;
    bool getSfntTableData(uint tag, uchar *buffer, uint *length) const Q_DECL_OVERRIDE;
    int synthesized() const Q_DECL_OVERRIDE;
    QFixed emSquareSize() const Q_DECL_OVERRIDE;

    glyph_t glyphIndex(uint ucs4) const Q_DECL_OVERRIDE;
    bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, ShaperFlags flags) const Q_DECL_OVERRIDE;
    void recalcAdvances(QGlyphLayout *glyphs, ShaperFlags) const Q_DECL_OVERRIDE;

    void addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path, QTextItem::RenderFlags flags) Q_DECL_OVERRIDE;
    virtual void addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nglyphs,
                         QPainterPath *path, QTextItem::RenderFlags flags);

    HGDIOBJ selectDesignFont() const;

    glyph_metrics_t boundingBox(const QGlyphLayout &glyphs) Q_DECL_OVERRIDE;
    glyph_metrics_t boundingBox(glyph_t g) Q_DECL_OVERRIDE { return boundingBox(g, QTransform()); }
    glyph_metrics_t boundingBox(glyph_t g, const QTransform &t) Q_DECL_OVERRIDE;


    QFixed ascent() const Q_DECL_OVERRIDE;
    QFixed descent() const Q_DECL_OVERRIDE;
    QFixed leading() const Q_DECL_OVERRIDE;
    QFixed xHeight() const Q_DECL_OVERRIDE;
    QFixed averageCharWidth() const Q_DECL_OVERRIDE;
    qreal maxCharWidth() const Q_DECL_OVERRIDE;
    qreal minLeftBearing() const Q_DECL_OVERRIDE;
    qreal minRightBearing() const Q_DECL_OVERRIDE;

    QImage alphaMapForGlyph(glyph_t t) Q_DECL_OVERRIDE { return alphaMapForGlyph(t, QTransform()); }
    QImage alphaMapForGlyph(glyph_t, const QTransform &xform) Q_DECL_OVERRIDE;
    QImage alphaRGBMapForGlyph(glyph_t t, QFixed subPixelPosition, const QTransform &xform) Q_DECL_OVERRIDE;
    glyph_metrics_t alphaMapBoundingBox(glyph_t glyph, QFixed, const QTransform &matrix, GlyphFormat) Q_DECL_OVERRIDE;

    QFontEngine *cloneWithSize(qreal pixelSize) const Q_DECL_OVERRIDE;
    bool supportsTransformation(const QTransform &transform) const Q_DECL_OVERRIDE;

#ifndef Q_CC_MINGW
    void getGlyphBearings(glyph_t glyph, qreal *leftBearing = 0, qreal *rightBearing = 0) Q_DECL_OVERRIDE;
#endif

    bool hasUnreliableGlyphOutline() const Q_DECL_OVERRIDE;

    int getGlyphIndexes(const QChar *ch, int numChars, QGlyphLayout *glyphs) const;
    void getCMap();

    bool getOutlineMetrics(glyph_t glyph, const QTransform &t, glyph_metrics_t *metrics) const;

    const QSharedPointer<QWindowsFontEngineData> &fontEngineData() const { return m_fontEngineData; }

    void setUniqueFamilyName(const QString &newName) { uniqueFamilyName = newName; }

private:
    QWindowsNativeImage *drawGDIGlyph(HFONT font, glyph_t, int margin, const QTransform &xform,
                                      QImage::Format mask_format);
    bool hasCFFTable() const;
    bool hasCMapTable() const;
    bool hasGlyfTable() const;
    bool hasEbdtTable() const;

    const QSharedPointer<QWindowsFontEngineData> m_fontEngineData;

    const QString     _name;
    QString     uniqueFamilyName;
    const HFONT       hfont;
    const LOGFONT     m_logfont;
    uint        stockFont  : 1;
    uint        ttf        : 1;
    uint        hasOutline : 1;
    uint        hasUnreliableOutline : 1;
    uint        cffTable   : 1;
    TEXTMETRIC  tm;
    int         lw;
    const unsigned char *cmap;
    QByteArray cmapTable;
    mutable qreal lbearing;
    mutable qreal rbearing;
    QFixed designToDevice;
    int unitsPerEm;
    QFixed x_height;
    FaceId _faceId;

    mutable int synthesized_flags;
    mutable QFixed lineWidth;
    mutable unsigned char *widthCache;
    mutable uint widthCacheSize;
    mutable QFixed *designAdvances;
    mutable int designAdvancesSize;
};


class QWindowsMultiFontEngine : public QFontEngineMultiBasicImpl
{
public:
    explicit QWindowsMultiFontEngine(QFontEngine *fe, int script);

    void loadEngine(int at);
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(HFONT)
Q_DECLARE_METATYPE(LOGFONT)

#endif // QWINDOWSFONTENGINE_H

