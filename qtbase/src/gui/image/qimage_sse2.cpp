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

#include "qimage.h"
#include <private/qimage_p.h>
#include <private/qsimd_p.h>
#include <private/qdrawhelper_p.h>
#include <private/qdrawingprimitive_sse2_p.h>

#ifdef QT_COMPILER_SUPPORTS_SSE2

QT_BEGIN_NAMESPACE

bool convert_ARGB_to_ARGB_PM_inplace_sse2(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_ARGB32);

    // extra pixels on each line
    const int spare = data->width & 3;
    // width in pixels of the pad at the end of each line
    const int pad = (data->bytes_per_line >> 2) - data->width;
    const int iter = data->width >> 2;
    int height = data->height;

    const __m128i alphaMask = _mm_set1_epi32(0xff000000);
    const __m128i nullVector = _mm_setzero_si128();
    const __m128i half = _mm_set1_epi16(0x80);
    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);

    __m128i *d = reinterpret_cast<__m128i*>(data->data);
    while (height--) {
        const __m128i *end = d + iter;

        for (; d != end; ++d) {
            const __m128i srcVector = _mm_loadu_si128(d);
            const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask);
            if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, alphaMask)) == 0xffff) {
                // opaque, data is unchanged
            } else if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, nullVector)) == 0xffff) {
                // fully transparent
                _mm_storeu_si128(d, nullVector);
            } else {
                __m128i alphaChannel = _mm_srli_epi32(srcVector, 24);
                alphaChannel = _mm_or_si128(alphaChannel, _mm_slli_epi32(alphaChannel, 16));

                __m128i result;
                BYTE_MUL_SSE2(result, srcVector, alphaChannel, colorMask, half);
                result = _mm_or_si128(_mm_andnot_si128(alphaMask, result), srcVectorAlpha);
                _mm_storeu_si128(d, result);
            }
        }

        QRgb *p = reinterpret_cast<QRgb*>(d);
        QRgb *pe = p+spare;
        for (; p != pe; ++p) {
            if (*p < 0x00ffffff)
                *p = 0;
            else if (*p < 0xff000000)
                *p = qPremultiply(*p);
        }

        d = reinterpret_cast<__m128i*>(p+pad);
    }

    data->format = QImage::Format_ARGB32_Premultiplied;
    return true;
}

QT_END_NAMESPACE

#endif // QT_COMPILER_SUPPORTS_SSE2
