
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef SkTextLayout_DEFINED
#define SkTextLayout_DEFINED

#include "SkPaint.h"
#include "SkRefCnt.h"

class SkTextStyle : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkTextStyle)

    SkTextStyle();
    SkTextStyle(const SkTextStyle&);
    explicit SkTextStyle(const SkPaint&);
    virtual ~SkTextStyle();

    const SkPaint& paint() const { return fPaint; }
    SkPaint& paint() { return fPaint; }

    // todo: bidi-override, language

private:
    SkPaint fPaint;

    typedef SkRefCnt INHERITED;
};

class SkTextLayout {
public:
    SkTextLayout();
    ~SkTextLayout();

    void setText(const char text[], size_t length);
    void setBounds(const SkRect& bounds);

    SkTextStyle* getDefaultStyle() const { return fDefaultStyle; }
    SkTextStyle* setDefaultStyle(SkTextStyle*);

//    SkTextStyle* setStyle(SkTextStyle*, size_t offset, size_t length);

    void draw(SkCanvas* canvas);

private:
    SkTDArray<char> fText;
    SkTextStyle*    fDefaultStyle;
    SkRect          fBounds;

    // cache
    struct Line;
    struct GlyphRun;
    SkTDArray<Line*> fLines;
};

#endif
