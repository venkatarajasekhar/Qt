/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qbig5codec_p.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_BIG_CODECS
static int qt_Big5hkscsToUnicode(const uchar *s, uint *pwc);
static int qt_UnicodeToBig5hkscs(uint wc, uchar *r);

#define InRange(c, lower, upper)  (((c) >= (lower)) && ((c) <= (upper)))
#define IsLatin(c)        ((c) < 0x80)
#define IsFirstByte(c)        (InRange((c), 0x81, 0xFE))
#define IsSecondByteRange1(c)        (InRange((c), 0x40, 0x7E))
#define IsSecondByteRange2(c)        (InRange((c), 0xA1, 0xFE))
#define IsSecondByte(c)        (IsSecondByteRange1(c) || IsSecondByteRange2(c))

#define        QValidChar(u)        ((u) ? QChar((ushort)(u)) : QChar(QChar::ReplacementCharacter))


int QBig5Codec::_mibEnum()
{
    return 2026;
}

QByteArray QBig5Codec::_name()
{
    return "Big5";
} Big 5 to Unicode maping tables.
// Tables are in sorted order on X(all table together) & Y(individually)


//All the tables  are sorted on both x and y, so can be used for binary search
static B5Index b5_map_table[5] = {
    {b5_8140_to_uc_map , sizeof(b5_8140_to_uc_map)/sizeof(B5Map)},
    {b5_8E40_to_uc_map , sizeof(b5_8E40_to_uc_map)/sizeof(B5Map)},
    {b5_C6A1_to_uc_map , sizeof(b5_C6A1_to_uc_map)/sizeof(B5Map)},
    {b5_FA40_to_uc_map , sizeof(b5_FA40_to_uc_map)/sizeof(B5Map)},
    {uc_to_b5_map , sizeof(uc_to_b5_map)/sizeof(B5Map)}
};

static int qt_Big5ToUnicode(const uchar *buf, uint *u)
{
    //for this conversion only first 4 tables are used.
    for(int i = 0; i < 4; i++) {
        int start = 0;
        int end = b5_map_table[i].tableSize - 1;

        uint b5 = (buf[0] << 8) + buf[1];
        while (start <= end) {
            int middle = (end + start + 1)/2;
            if (b5_map_table[i].table[middle].x == b5) {
                *u = b5_map_table[i].table[middle].y;
                return 2;
            } else if (b5_map_table[i].table[middle].x > b5) {
                end = middle - 1;
            } else {
                start = middle + 1;
            }
        }
    }
    return qt_Big5hkscsToUnicode(buf, u);
}

static int qt_UnicodeToBig5(ushort ch, uchar *buf)
{
    //all the tables are individually sorted on Y
    for(int i = 0; i < 5; i++) {
        int start = 0;
        int end = b5_map_table[i].tableSize - 1;

        while (start <= end) {
            int middle = (end + start + 1)/2;
            if (b5_map_table[i].table[middle].y == ch) {
                buf[0] = b5_map_table[i].table[middle].x >> 8;
                buf[1] = b5_map_table[i].table[middle].x & 0xff;
                return 2;
            } else if (b5_map_table[i].table[middle].y > ch) {
                end = middle - 1;
            } else {
                start = middle + 1;
            }
        }
    }
    return qt_UnicodeToBig5hkscs(ch, buf);
}

QString QBig5Codec::convertToUnicode(const char* chars, int len, ConverterState *state) const
{
    QChar replacement = QChar::ReplacementCharacter;
    uchar buf[2] = {0};
    int nbuf = 0;
    if (state) {
        if (state->flags & ConvertInvalidToNull)
            replacement = QChar::Null;
        nbuf = state->remainingChars;
        buf[0] = state->state_data[0];
        buf[1] = state->state_data[1];
    }
    int invalid = 0;

    //qDebug("QBig5Codec::toUnicode(const char* chars = \"%s\", int len = %d)", chars, len);
    QString result;
    for (int i=0; i<len; i++) {
        uchar ch = chars[i];
        switch (nbuf) {
        case 0:
            if (IsLatin(ch)) {
                // ASCII
                result += QLatin1Char(ch);
            } else if (IsFirstByte(ch)) {
                // Big5-ETen
                buf[0] = ch;
                nbuf = 1;
            } else {
                // Invalid
                result += replacement;
                ++invalid;
            }
            break;
        case 1:
            if (IsSecondByte(ch)) {
                // Big5-ETen
                uint u;
                buf[1] = ch;
                if (qt_Big5ToUnicode(buf, &u) == 2)
                    result += QValidChar(u);
                else {
                    // Error
                    result += replacement;
                    ++invalid;
                }
            } else {
                // Error
                result += replacement;
                ++invalid;
            }
            nbuf = 0;
            break;
        }
    }
    if (state) {
        state->remainingChars = nbuf;
        state->state_data[0] = buf[0];
        state->state_data[1] = buf[1];
        state->invalidChars += invalid;
    }
    return result;
}

QByteArray QBig5Codec::convertFromUnicode(const QChar *uc, int len, ConverterState *state) const
{
    char replacement = '?';
    if (state) {
        if (state->flags & ConvertInvalidToNull)
            replacement = 0;
    }

    int invalid = 0;

    int rlen = 2*len + 1;
    QByteArray rstr;
    rstr.resize(rlen);

    uchar* cursor = (uchar*)rstr.data();
    for (int i=0; i<len; i++) {
        ushort ch = uc[i].unicode();
        uchar c[2];
        if (ch < 0x80) {
            // ASCII
            *cursor++ = ch;
        } else if (qt_UnicodeToBig5(ch, c) == 2 && c[0] >= 0xa1 && c[0] <= 0xf9) {
            *cursor++ = c[0];
            *cursor++ = c[1];
        } else {
            *cursor++ = replacement;
            ++invalid;
        }
    }
    rstr.resize(cursor - (uchar*)rstr.constData());

    if (state) {
        state->invalidChars += invalid;
    }
    return rstr;
}

QList<QByteArray> QBig5Codec::_aliases()
{
    QList<QByteArray> aliases;
    aliases += "Big5-ETen";
    aliases += "CP950";
    return aliases;
}

int QBig5hkscsCodec::_mibEnum()
{
    return 2101;
}


QByteArray QBig5hkscsCodec::_name()
{
    return "Big5-HKSCS";
}


QString QBig5hkscsCodec::convertToUnicode(const char* chars, int len, ConverterState *state) const
{
    uchar buf[2] = {0};
    int nbuf = 0;
    QChar replacement = QChar::ReplacementCharacter;
    if (state) {
        if (state->flags & ConvertInvalidToNull)
            replacement = QChar::Null;
        nbuf = state->remainingChars;
        buf[0] = state->state_data[0];
        buf[1] = state->state_data[1];
    }
    int invalid = 0;

    //qDebug("QBig5hkscsCodec::toUnicode(const char* chars = \"%s\", int len = %d)", chars, len);
    QString result;
    for (int i=0; i<len; i++) {
        uchar ch = chars[i];
        switch (nbuf) {
        case 0:
            if (IsLatin(ch)) {
                // ASCII
                result += QLatin1Char(ch);
            } else if (IsFirstByte(ch)) {
                // Big5-HKSCS
                buf[0] = ch;
                nbuf = 1;
            } else {
                // Invalid
                result += replacement;
                ++invalid;
            }
            break;
        case 1:
            if (IsSecondByte(ch)) {
                // Big5-HKSCS
                uint u;
                buf[1] = ch;
                if (qt_Big5hkscsToUnicode(buf, &u) == 2)
                    result += QValidChar(u);
                else {
                    // Error
                    result += replacement;
                    ++invalid;
                }
            } else {
                // Error
                result += replacement;
                ++invalid;
            }
            nbuf = 0;
            break;
        }
    }
    if (state) {
        state->remainingChars = nbuf;
        state->state_data[0] = buf[0];
        state->state_data[1] = buf[1];
        state->invalidChars += invalid;
    }
    return result;
}


QByteArray QBig5hkscsCodec::convertFromUnicode(const QChar *uc, int len, ConverterState *state) const
{
    char replacement = '?';
    if (state) {
        if (state->flags & ConvertInvalidToNull)
            replacement = 0;
    }

    int invalid = 0;

    int rlen = 2*len + 1;
    QByteArray rstr;
    rstr.resize(rlen);
    uchar* cursor = (uchar*)rstr.data();
    for (int i = 0; i < len; i++) {
        unsigned short ch = uc[i].unicode();
        uchar c[2];
        if (ch < 0x80) {
            // ASCII
            *cursor++ = ch;
        } else if (qt_UnicodeToBig5hkscs(ch, c) == 2) {
            // Big5-HKSCS
            *cursor++ = c[0];
            *cursor++ = c[1];
        } else {
            // Error
            *cursor++ = replacement;
        }
    }
    rstr.resize(cursor - (uchar*)rstr.constData());

    if (state) {
        state->invalidChars += invalid;
    }
    return rstr;
}


/* ====================================================================== */

/*
 * big5hkscs to ucs4 convert routing
 */
int qt_UnicodeToBig5hkscs (uint wc, uchar *r)
{
    const Summary16 *summary = NULL;
    if (wc < 0x80) {
        r[0] = (uchar) wc;
        return 1;
    }
    if (wc < 0x0460)
        summary = &big5hkscs_uni2index_page00[(wc>>4)];
    else if (wc >= 0x1e00 && wc < 0x1ed0)
        summary = &big5hkscs_uni2index_page1e[(wc>>4)-0x1e0];
    else if (wc >= 0x2000 && wc < 0x2740)
        summary = &big5hkscs_uni2index_page20[(wc>>4)-0x200];
    else if (wc >= 0x2e00 && wc < 0x9fb0)
        summary = &big5hkscs_uni2index_page2e[(wc>>4)-0x2e0];
    else if (wc >= 0xe000 && wc < 0xfa30)
        summary = &big5hkscs_uni2index_pagee0[(wc>>4)-0xe00];
    else if (wc >= 0xfe00 && wc < 0xfff0)
        summary = &big5hkscs_uni2index_pagefe[(wc>>4)-0xfe0];
    else if (wc >= 0x20000 && wc < 0x291f0)
        summary = &big5hkscs_uni2index_page200[(wc>>4)-0x2000];
    else if (wc >= 0x29400 && wc < 0x29600)
        summary = &big5hkscs_uni2index_page294[(wc>>4)-0x2940];
    else if (wc >= 0x29700 && wc < 0x2a6b0)
        summary = &big5hkscs_uni2index_page297[(wc>>4)-0x2970];
    else if (wc >= 0x2f800 && wc < 0x2f9e0)
        summary = &big5hkscs_uni2index_page2f8[(wc>>4)-0x2f80];
    if (summary) {
        ushort used = summary->used;
        uint i = wc & 0x0f;
        if (used & ((ushort) 1 << i)) {
            const uchar *c;
            /* Keep in `used' only the bits 0..i-1. */
            used &= ((ushort) 1 << i) - 1;
            /* Add `summary->index' and the number of bits set in `used'. */

            used = (used & 0x5555) + ((used & 0xaaaa) >> 1);
            used = (used & 0x3333) + ((used & 0xcccc) >> 2);
            used = (used & 0x0f0f) + ((used & 0xf0f0) >> 4);
            used = (used & 0x00ff) + (used >> 8);
            c = big5hkscs_to_charset[summary->index + used];
            if (c [1] != 0) {
                r[0] = c[0];
                r[1] = c[1];
                return 2;
            } else {  // (c [1] == 0)
                r[0] = c[0];
                return 1;
            }
        }
    }
    return 0;
}


/* ====================================================================== */
#endif // QT_NO_BIG_CODECS

QT_END_NAMESPACE
