/*
 * Copyright 2010 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if NOUVEAU_DRIVER == 0xc0
# include "nvc0_screen.h"
# include "nvc0_3d.xml.h"
#else
# include "nv50_screen.h"
# include "nv50_3d.xml.h"
#endif
#include "nv50_texture.xml.h"
#include "nv50_defs.xml.h"

#include "pipe/p_defines.h"

/* Abbreviated usage masks:
 * T: texturing
 * R: render target
 * B: render target, blendable
 * C: render target (color), blendable only on nvc0
 * D: scanout/display target, blendable
 * Z: depth/stencil
 * V: vertex fetch
 */
#define U_V   PIPE_BIND_VERTEX_BUFFER
#define U_T   PIPE_BIND_SAMPLER_VIEW
#define U_TR  PIPE_BIND_RENDER_TARGET | U_T
#define U_TB  PIPE_BIND_BLENDABLE | U_TR
#define U_TD  PIPE_BIND_SCANOUT | PIPE_BIND_DISPLAY_TARGET | U_TB
#define U_TZ  PIPE_BIND_DEPTH_STENCIL | U_T
#define U_TV  U_V | U_T
#define U_TRV U_V | U_TR
#define U_TBV U_V | U_TB
#define U_TDV U_V | U_TD
#if NOUVEAU_DRIVER == 0xc0
# define U_TC  U_TB
# define U_TCV U_TBV
#else
# define U_TC  U_TR
# define U_TCV U_TRV
#endif

#define NV50_SURFACE_FORMAT_NONE 0
#define NV50_ZETA_FORMAT_NONE 0

/* for vertex buffers: */
#define NV50_TIC_0_FMT_8_8_8    NV50_TIC_0_FMT_8_8_8_8
#define NV50_TIC_0_FMT_16_16_16 NV50_TIC_0_FMT_16_16_16_16
#define NV50_TIC_0_FMT_32_32_32 NV50_TIC_0_FMT_32_32_32_32

#if NOUVEAU_DRIVER == 0xc0
# define NVXX_3D_VAF_SIZE(s) NVC0_3D_VERTEX_ATTRIB_FORMAT_SIZE_##s
# define NVXX_3D_VAF_TYPE(t) NVC0_3D_VERTEX_ATTRIB_FORMAT_TYPE_##t
#else
# define NVXX_3D_VAF_SIZE(s) NV50_3D_VERTEX_ARRAY_ATTRIB_FORMAT_##s
# define NVXX_3D_VAF_TYPE(t) NV50_3D_VERTEX_ARRAY_ATTRIB_TYPE_##t
#endif

#define TBLENT_A_(pf, sf, r, g, b, a, t0, t1, t2, t3, sz, u, br)        \
   [PIPE_FORMAT_##pf] = {                                               \
      sf,                                                               \
      (NV50_TIC_MAP_##r << NV50_TIC_0_MAPR__SHIFT) |                    \
      (NV50_TIC_MAP_##g << NV50_TIC_0_MAPG__SHIFT) |                    \
      (NV50_TIC_MAP_##b << NV50_TIC_0_MAPB__SHIFT) |                    \
      (NV50_TIC_MAP_##a << NV50_TIC_0_MAPA__SHIFT) |                    \
      (NV50_TIC_TYPE_##t0 << NV50_TIC_0_TYPE0__SHIFT) |                 \
      (NV50_TIC_TYPE_##t1 << NV50_TIC_0_TYPE1__SHIFT) |                 \
      (NV50_TIC_TYPE_##t2 << NV50_TIC_0_TYPE2__SHIFT) |                 \
      (NV50_TIC_TYPE_##t3 << NV50_TIC_0_TYPE3__SHIFT) |                 \
      NV50_TIC_0_FMT_##sz,                                              \
      NVXX_3D_VAF_SIZE(sz) |                                            \
      NVXX_3D_VAF_TYPE(t0) | (br << 31),                                \
      U_##u                                                             \
   }

#define TBLENT_B_(pf, sf, r, g, b, a, t0, t1, t2, t3, sz, u)            \
   [PIPE_FORMAT_##pf] = {                                               \
      sf,                                                               \
      (NV50_TIC_MAP_##r << NV50_TIC_0_MAPR__SHIFT) |                    \
      (NV50_TIC_MAP_##g << NV50_TIC_0_MAPG__SHIFT) |                    \
      (NV50_TIC_MAP_##b << NV50_TIC_0_MAPB__SHIFT) |                    \
      (NV50_TIC_MAP_##a << NV50_TIC_0_MAPA__SHIFT) |                    \
      (NV50_TIC_TYPE_##t0 << NV50_TIC_0_TYPE0__SHIFT) |                 \
      (NV50_TIC_TYPE_##t1 << NV50_TIC_0_TYPE1__SHIFT) |                 \
      (NV50_TIC_TYPE_##t2 << NV50_TIC_0_TYPE2__SHIFT) |                 \
      (NV50_TIC_TYPE_##t3 << NV50_TIC_0_TYPE3__SHIFT) |                 \
      NV50_TIC_0_FMT_##sz, 0, U_##u                                     \
   }

#define C4A(p, n, r, g, b, a, t, s, u, br)                              \
   TBLENT_A_(p, NV50_SURFACE_FORMAT_##n, r, g, b, a, t, t, t, t, s, u, br)
#define C4B(p, n, r, g, b, a, t, s, u)                                  \
   TBLENT_B_(p, NV50_SURFACE_FORMAT_##n, r, g, b, a, t, t, t, t, s, u)

#define ZXB(p, n, r, g, b, a, t, s, u)                                  \
   TBLENT_B_(p, NV50_ZETA_FORMAT_##n,                                   \
             r, g, b, ONE_FLOAT, t, UINT, UINT, UINT, s, u)
#define ZSB(p, n, r, g, b, a, t, s, u)                                  \
   TBLENT_B_(p, NV50_ZETA_FORMAT_##n,                                   \
             r, g, b, ONE_FLOAT, t, UINT, UINT, UINT, s, u)
#define SZB(p, n, r, g, b, a, t, s, u)                                  \
   TBLENT_B_(p, NV50_ZETA_FORMAT_##n,                                   \
             r, g, b, ONE_FLOAT, UINT, t, UINT, UINT, s, u)

#define F3A(p, n, r, g, b, a, t, s, u)          \
   C4A(p, n, r, g, b, ONE_FLOAT, t, s, u, 0)
#define I3A(p, n, r, g, b, a, t, s, u)          \
   C4A(p, n, r, g, b, ONE_INT, t, s, u, 0)
#define F3B(p, n, r, g, b, a, t, s, u)          \
   C4B(p, n, r, g, b, ONE_FLOAT, t, s, u)
#define I3B(p, n, r, g, b, a, t, s, u)          \
   C4B(p, n, r, g, b, ONE_INT, t, s, u)

#define F2A(p, n, r, g, b, a, t, s, u)          \
   C4A(p, n, r, g, ZERO, ONE_FLOAT, t, s, u, 0)
#define I2A(p, n, r, g, b, a, t, s, u)          \
   C4A(p, n, r, g, ZERO, ONE_INT, t, s, u, 0)
#define F2B(p, n, r, g, b, a, t, s, u)          \
   C4B(p, n, r, g, ZERO, ONE_FLOAT, t, s, u)
#define I2B(p, n, r, g, b, a, t, s, u)          \
   C4B(p, n, r, g, ZERO, ONE_INT, t, s, u)

#define F1A(p, n, r, g, b, a, t, s, u)             \
   C4A(p, n, r, ZERO, ZERO, ONE_FLOAT, t, s, u, 0)
#define I1A(p, n, r, g, b, a, t, s, u)             \
   C4A(p, n, r, ZERO, ZERO, ONE_INT, t, s, u, 0)
#define F1B(p, n, r, g, b, a, t, s, u)          \
   C4B(p, n, r, ZERO, ZERO, ONE_FLOAT, t, s, u)
#define I1B(p, n, r, g, b, a, t, s, u)          \
   C4B(p, n, r, ZERO, ZERO, ONE_INT, t, s, u)

#define A1B(p, n, r, g, b, a, t, s, u)          \
   C4B(p, n, ZERO, ZERO, ZERO, a, t, s, u)

#if NOUVEAU_DRIVER == 0xc0
const struct nvc0_format nvc0_format_table[PIPE_FORMAT_COUNT] =
#else
const struct nv50_format nv50_format_table[PIPE_FORMAT_COUNT] =
#endif
{
   C4A(B8G8R8A8_UNORM, BGRA8_UNORM, C2, C1, C0, C3, UNORM, 8_8_8_8, TDV, 1),
   F3A(B8G8R8X8_UNORM, BGRX8_UNORM, C2, C1, C0, xx, UNORM, 8_8_8_8, TD),
   C4A(B8G8R8A8_SRGB, BGRA8_SRGB, C2, C1, C0, C3, UNORM, 8_8_8_8, TD, 1),
   F3A(B8G8R8X8_SRGB, BGRX8_SRGB, C2, C1, C0, xx, UNORM, 8_8_8_8, TD),
   C4A(R8G8B8A8_UNORM, RGBA8_UNORM, C0, C1, C2, C3, UNORM, 8_8_8_8, TBV, 0),
   F3A(R8G8B8X8_UNORM, RGBX8_UNORM, C0, C1, C2, xx, UNORM, 8_8_8_8, TB),
   C4A(R8G8B8A8_SRGB, RGBA8_SRGB, C0, C1, C2, C3, UNORM, 8_8_8_8, TB, 0),

   ZXB(Z16_UNORM, Z16_UNORM, C0, C0, C0, xx, UNORM, Z16, TZ),
   ZXB(Z32_FLOAT, Z32_FLOAT, C0, C0, C0, xx, FLOAT, Z32, TZ),
   ZXB(Z24X8_UNORM, Z24_X8_UNORM, C0, C0, C0, xx, UNORM, Z24_X8, TZ),
   ZSB(Z24_UNORM_S8_UINT, Z24_S8_UNORM, C0, C0, C0, xx, UNORM, Z24_S8, TZ),
   ZSB(X24S8_UINT, NONE, C1, C1, C1, xx, UNORM, Z24_S8, T),
   SZB(S8_UINT_Z24_UNORM, S8_Z24_UNORM, C1, C1, C1, xx, UNORM, S8_Z24, TZ),
   SZB(S8X24_UINT, NONE, C0, C0, C0, xx, UNORM, S8_Z24, T),
   ZSB(Z32_FLOAT_S8X24_UINT, Z32_S8_X24_FLOAT, C0, C0, C0, xx, FLOAT,
       Z32_S8_X24, TZ),
   ZSB(X32_S8X24_UINT, NONE, C1, C1, C1, xx, FLOAT, Z32_S8_X24, T),

   F3B(B5G6R5_UNORM, B5G6R5_UNORM, C2, C1, C0, xx, UNORM, 5_6_5, T),
   C4B(B5G5R5A1_UNORM, BGR5_A1_UNORM, C2, C1, C0, C3, UNORM, 5_5_5_1, TB),
   F3B(B5G5R5X1_UNORM, BGR5_X1_UNORM, C2, C1, C0, xx, UNORM, 5_5_5_1, TB),
   C4B(B4G4R4A4_UNORM, NONE, C2, C1, C0, C3, UNORM, 4_4_4_4, T),
   F3B(B4G4R4X4_UNORM, NONE, C2, C1, C0, xx, UNORM, 4_4_4_4, T),
   F3B(R9G9B9E5_FLOAT, NONE, C0, C1, C2, xx, FLOAT, 9_9_9_E5, T),

   C4A(R10G10B10A2_UNORM, RGB10_A2_UNORM, C0, C1, C2, C3, UNORM, 10_10_10_2,
       TBV, 0),
   C4A(B10G10R10A2_UNORM, BGR10_A2_UNORM, C2, C1, C0, C3, UNORM, 10_10_10_2,
       TBV, 1),
   C4A(R10G10B10A2_SNORM, NONE, C0, C1, C2, C3, SNORM, 10_10_10_2, TV, 0),
   C4A(B10G10R10A2_SNORM, NONE, C2, C1, C0, C3, SNORM, 10_10_10_2, TV, 1),

   F3B(R11G11B10_FLOAT, R11G11B10_FLOAT, C0, C1, C2, xx, FLOAT, 11_11_10, TB),

   F3B(L8_UNORM, R8_UNORM, C0, C0, C0, xx, UNORM, 8, TB),
   F3B(L8_SRGB, R8_UNORM, C0, C0, C0, xx, UNORM, 8, TB),
   F3B(L8_SNORM, R8_SNORM, C0, C0, C0, xx, SNORM, 8, TC),
   F3B(L8_SINT, R8_SINT, C0, C0, C0, xx, SINT, 8, TR),
   F3B(L8_UINT, R8_UINT, C0, C0, C0, xx, UINT, 8, TR),
   F3B(L16_UNORM, R16_UNORM, C0, C0, C0, xx, UNORM, 16, TC),
   F3B(L16_SNORM, R16_SNORM, C0, C0, C0, xx, SNORM, 16, TC),
   F3B(L16_FLOAT, R16_FLOAT, C0, C0, C0, xx, FLOAT, 16, TB),
   F3B(L16_SINT, R16_SINT, C0, C0, C0, xx, SINT, 16, TR),
   F3B(L16_UINT, R16_UINT, C0, C0, C0, xx, UINT, 16, TR),
   F3B(L32_FLOAT, R32_FLOAT, C0, C0, C0, xx, FLOAT, 32, TB),
   F3B(L32_SINT, R32_SINT, C0, C0, C0, xx, SINT, 32, TR),
   F3B(L32_UINT, R32_UINT, C0, C0, C0, xx, UINT, 32, TR),

   C4B(I8_UNORM, R8_UNORM, C0, C0, C0, C0, UNORM, 8, TR),
   C4B(I8_SNORM, R8_SNORM, C0, C0, C0, C0, SNORM, 8, TR),
   C4B(I8_SINT, R8_SINT, C0, C0, C0, C0, SINT, 8, TR),
   C4B(I8_UINT, R8_UINT, C0, C0, C0, C0, UINT, 8, TR),
   C4B(I16_UNORM, R16_UNORM, C0, C0, C0, C0, UNORM, 16, TR),
   C4B(I16_SNORM, R16_SNORM, C0, C0, C0, C0, SNORM, 16, TR),
   C4B(I16_FLOAT, R16_FLOAT, C0, C0, C0, C0, FLOAT, 16, TR),
   C4B(I16_SINT, R16_SINT, C0, C0, C0, C0, SINT, 16, TR),
   C4B(I16_UINT, R16_UINT, C0, C0, C0, C0, UINT, 16, TR),
   C4B(I32_FLOAT, R32_FLOAT, C0, C0, C0, C0, FLOAT, 32, TR),
   C4B(I32_SINT, R32_SINT, C0, C0, C0, C0, SINT, 32, TR),
   C4B(I32_UINT, R32_UINT, C0, C0, C0, C0, UINT, 32, TR),

   A1B(A8_UNORM, A8_UNORM, xx, xx, xx, C0, UNORM, 8, TB),
   A1B(A8_SNORM, R8_SNORM, xx, xx, xx, C0, SNORM, 8, T),
   A1B(A8_SINT, R8_SINT, xx, xx, xx, C0, SINT, 8, T),
   A1B(A8_UINT, R8_UINT, xx, xx, xx, C0, UINT, 8, T),
   A1B(A16_UNORM, R16_UNORM, xx, xx, xx, C0, UNORM, 16, T),
   A1B(A16_SNORM, R16_SNORM, xx, xx, xx, C0, SNORM, 16, T),
   A1B(A16_FLOAT, R16_FLOAT, xx, xx, xx, C0, FLOAT, 16, T),
   A1B(A16_SINT, R16_SINT, xx, xx, xx, C0, SINT, 16, T),
   A1B(A16_UINT, R16_UINT, xx, xx, xx, C0, UINT, 16, T),
   A1B(A32_FLOAT, R32_FLOAT, xx, xx, xx, C0, FLOAT, 32, T),
   A1B(A32_SINT, R32_SINT, xx, xx, xx, C0, SINT, 32, T),
   A1B(A32_UINT, R32_UINT, xx, xx, xx, C0, UINT, 32, T),

   C4B(L4A4_UNORM, NONE, C0, C0, C0, C1, UNORM, 4_4, T),
   C4B(L8A8_UNORM, RG8_UNORM, C0, C0, C0, C1, UNORM, 8_8, T),
   C4B(L8A8_SNORM, RG8_SNORM, C0, C0, C0, C1, SNORM, 8_8, T),
   C4B(L8A8_SRGB, RG8_UNORM, C0, C0, C0, C1, UNORM, 8_8, T),
   C4B(L8A8_SINT, RG8_SINT, C0, C0, C0, C1, SINT, 8_8, T),
   C4B(L8A8_UINT, RG8_UINT, C0, C0, C0, C1, UINT, 8_8, T),
   C4B(L16A16_UNORM, RG16_UNORM, C0, C0, C0, C1, UNORM, 16_16, T),
   C4B(L16A16_SNORM, RG16_SNORM, C0, C0, C0, C1, SNORM, 16_16, T),
   C4B(L16A16_FLOAT, RG16_FLOAT, C0, C0, C0, C1, FLOAT, 16_16, T),
   C4B(L16A16_SINT, RG16_SINT, C0, C0, C0, C1, SINT, 16_16, T),
   C4B(L16A16_UINT, RG16_UINT, C0, C0, C0, C1, UINT, 16_16, T),
   C4B(L32A32_FLOAT, RG32_FLOAT, C0, C0, C0, C1, FLOAT, 32_32, T),
   C4B(L32A32_SINT, RG32_SINT, C0, C0, C0, C1, SINT, 32_32, T),
   C4B(L32A32_UINT, RG32_UINT, C0, C0, C0, C1, UINT, 32_32, T),

   F3B(DXT1_RGB,   NONE, C0, C1, C2, xx, UNORM, DXT1, T),
   F3B(DXT1_SRGB,  NONE, C0, C1, C2, xx, UNORM, DXT1, T),
   C4B(DXT1_RGBA,  NONE, C0, C1, C2, C3, UNORM, DXT1, T),
   C4B(DXT1_SRGBA, NONE, C0, C1, C2, C3, UNORM, DXT1, T),
   C4B(DXT3_RGBA,  NONE, C0, C1, C2, C3, UNORM, DXT3, T),
   C4B(DXT3_SRGBA, NONE, C0, C1, C2, C3, UNORM, DXT3, T),
   C4B(DXT5_RGBA,  NONE, C0, C1, C2, C3, UNORM, DXT5, T),
   C4B(DXT5_SRGBA, NONE, C0, C1, C2, C3, UNORM, DXT5, T),

   F1B(RGTC1_UNORM, NONE, C0, xx, xx, xx, UNORM, RGTC1, T),
   F1B(RGTC1_SNORM, NONE, C0, xx, xx, xx, SNORM, RGTC1, T),
   F2B(RGTC2_UNORM, NONE, C0, C1, xx, xx, UNORM, RGTC2, T),
   F2B(RGTC2_SNORM, NONE, C0, C1, xx, xx, SNORM, RGTC2, T),
   F3B(LATC1_UNORM, NONE, C0, C0, C0, xx, UNORM, RGTC1, T),
   F3B(LATC1_SNORM, NONE, C0, C0, C0, xx, SNORM, RGTC1, T),
   C4B(LATC2_UNORM, NONE, C0, C0, C0, C1, UNORM, RGTC2, T),
   C4B(LATC2_SNORM, NONE, C0, C0, C0, C1, SNORM, RGTC2, T),

   C4A(R32G32B32A32_FLOAT, RGBA32_FLOAT, C0, C1, C2, C3, FLOAT, 32_32_32_32,
       TBV, 0),
   C4A(R32G32B32A32_UNORM, NONE, C0, C1, C2, C3, UNORM, 32_32_32_32, TV, 0),
   C4A(R32G32B32A32_SNORM, NONE, C0, C1, C2, C3, SNORM, 32_32_32_32, TV, 0),
   C4A(R32G32B32A32_SINT, RGBA32_SINT, C0, C1, C2, C3, SINT, 32_32_32_32,
       TRV, 0),
   C4A(R32G32B32A32_UINT, RGBA32_UINT, C0, C1, C2, C3, UINT, 32_32_32_32,
       TRV, 0),

   F2A(R32G32_FLOAT, RG32_FLOAT, C0, C1, xx, xx, FLOAT, 32_32, TBV),
   F2A(R32G32_UNORM, NONE, C0, C1, xx, xx, UNORM, 32_32, TV),
   F2A(R32G32_SNORM, NONE, C0, C1, xx, xx, SNORM, 32_32, TV),
   I2A(R32G32_SINT, RG32_SINT, C0, C1, xx, xx, SINT, 32_32, TRV),
   I2A(R32G32_UINT, RG32_UINT, C0, C1, xx, xx, UINT, 32_32, TRV),

   F1A(R32_FLOAT, R32_FLOAT, C0, xx, xx, xx, FLOAT, 32, TBV),
   F1A(R32_UNORM, NONE, C0, xx, xx, xx, UNORM, 32, TV),
   F1A(R32_SNORM, NONE, C0, xx, xx, xx, SNORM, 32, TV),
   I1A(R32_SINT, R32_SINT, C0, xx, xx, xx, SINT, 32, TRV),
   I1A(R32_UINT, R32_UINT, C0, xx, xx, xx, UINT, 32, TRV),

   C4A(R16G16B16A16_FLOAT, RGBA16_FLOAT, C0, C1, C2, C3, FLOAT, 16_16_16_16,
       TBV, 0),
   C4A(R16G16B16A16_UNORM, RGBA16_UNORM, C0, C1, C2, C3, UNORM, 16_16_16_16,
       TCV, 0),
   C4A(R16G16B16A16_SNORM, RGBA16_SNORM, C0, C1, C2, C3, SNORM, 16_16_16_16,
       TCV, 0),
   C4A(R16G16B16A16_SINT, RGBA16_SINT, C0, C1, C2, C3, SINT, 16_16_16_16,
       TRV, 0),
   C4A(R16G16B16A16_UINT, RGBA16_UINT, C0, C1, C2, C3, UINT, 16_16_16_16,
       TRV, 0),

   F2A(R16G16_FLOAT, RG16_FLOAT, C0, C1, xx, xx, FLOAT, 16_16, TBV),
   F2A(R16G16_UNORM, RG16_UNORM, C0, C1, xx, xx, UNORM, 16_16, TCV),
   F2A(R16G16_SNORM, RG16_SNORM, C0, C1, xx, xx, SNORM, 16_16, TCV),
   I2A(R16G16_SINT, RG16_SINT, C0, C1, xx, xx, SINT, 16_16, TRV),
   I2A(R16G16_UINT, RG16_UINT, C0, C1, xx, xx, UINT, 16_16, TRV),

   F1A(R16_FLOAT, R16_FLOAT, C0, xx, xx, xx, FLOAT, 16, TBV),
   F1A(R16_UNORM, R16_UNORM, C0, xx, xx, xx, UNORM, 16, TCV),
   F1A(R16_SNORM, R16_SNORM, C0, xx, xx, xx, SNORM, 16, TCV),
   I1A(R16_SINT, R16_SINT, C0, xx, xx, xx, SINT, 16, TRV),
   I1A(R16_UINT, R16_UINT, C0, xx, xx, xx, UINT, 16, TRV),

   C4A(R8G8B8A8_SNORM, RGBA8_SNORM, C0, C1, C2, C3, SNORM, 8_8_8_8, TCV, 0),
   C4A(R8G8B8A8_SINT, RGBA8_SINT, C0, C1, C2, C3, SINT, 8_8_8_8, TRV, 0),
   C4A(R8G8B8A8_UINT, RGBA8_UINT, C0, C1, C2, C3, UINT, 8_8_8_8, TRV, 0),

   F2A(R8G8_UNORM, RG8_UNORM, C0, C1, xx, xx, UNORM, 8_8, TBV),
   F2A(R8G8_SNORM, RG8_SNORM, C0, C1, xx, xx, SNORM, 8_8, TCV),
   I2A(R8G8_SINT, RG8_SINT, C0, C1, xx, xx, SINT, 8_8, TRV),
   I2A(R8G8_UINT, RG8_UINT, C0, C1, xx, xx, UINT, 8_8, TRV),

   F1A(R8_UNORM, R8_UNORM, C0, xx, xx, xx, UNORM, 8, TBV),
   F1A(R8_SNORM, R8_SNORM, C0, xx, xx, xx, SNORM, 8, TCV),
   I1A(R8_SINT, R8_SINT, C0, xx, xx, xx, SINT, 8, TRV),
   I1A(R8_UINT, R8_UINT, C0, xx, xx, xx, UINT, 8, TRV),

   F3B(R8G8_B8G8_UNORM, NONE, C0, C1, C2, xx, UNORM, U8_YA8_V8_YB8, T),
   F3B(G8R8_B8R8_UNORM, NONE, C1, C0, C2, xx, UNORM, U8_YA8_V8_YB8, T),
   F3B(G8R8_G8B8_UNORM, NONE, C0, C1, C2, xx, UNORM, YA8_U8_YB8_V8, T),
   F3B(R8G8_R8B8_UNORM, NONE, C1, C0, C2, xx, UNORM, YA8_U8_YB8_V8, T),

   F1B(R1_UNORM, BITMAP, C0, xx, xx, xx, UNORM, BITMAP, T),

   C4B(R4A4_UNORM, NONE, C0, ZERO, ZERO, C1, UNORM, 4_4, T),
   C4B(R8A8_UNORM, NONE, C0, ZERO, ZERO, C1, UNORM, 8_8, T),
   C4B(A4R4_UNORM, NONE, C1, ZERO, ZERO, C0, UNORM, 4_4, T),
   C4B(A8R8_UNORM, NONE, C1, ZERO, ZERO, C0, UNORM, 8_8, T),

   TBLENT_B_(R8SG8SB8UX8U_NORM, 0,
             C0, C1, C2, ONE_FLOAT, SNORM, SNORM, UNORM, UNORM, 8_8_8_8, T),
   TBLENT_B_(R5SG5SB6U_NORM, 0,
             C0, C1, C2, ONE_FLOAT, SNORM, SNORM, UNORM, UNORM, 5_5_6, T),

   /* vertex-only formats: */

   C4A(R32G32B32A32_SSCALED, NONE, C0, C1, C2, C3, SSCALED, 32_32_32_32, V, 0),
   C4A(R32G32B32A32_USCALED, NONE, C0, C1, C2, C3, USCALED, 32_32_32_32, V, 0),
   F3A(R32G32B32_FLOAT, NONE, C0, C1, C2, xx, FLOAT, 32_32_32, V),
   F3A(R32G32B32_UNORM, NONE, C0, C1, C2, xx, UNORM, 32_32_32, V),
   F3A(R32G32B32_SNORM, NONE, C0, C1, C2, xx, SNORM, 32_32_32, V),
   I3A(R32G32B32_SINT, NONE, C0, C1, C2, xx, SINT, 32_32_32, V),
   I3A(R32G32B32_UINT, NONE, C0, C1, C2, xx, UINT, 32_32_32, V),
   F3A(R32G32B32_SSCALED, NONE, C0, C1, C2, xx, SSCALED, 32_32_32, V),
   F3A(R32G32B32_USCALED, NONE, C0, C1, C2, xx, USCALED, 32_32_32, V),
   F2A(R32G32_SSCALED, NONE, C0, C1, xx, xx, SSCALED, 32_32, V),
   F2A(R32G32_USCALED, NONE, C0, C1, xx, xx, USCALED, 32_32, V),
   F1A(R32_SSCALED, NONE, C0, xx, xx, xx, SSCALED, 32, V),
   F1A(R32_USCALED, NONE, C0, xx, xx, xx, USCALED, 32, V),

   C4A(R16G16B16A16_SSCALED, NONE, C0, C1, C2, C3, SSCALED, 16_16_16_16, V, 0),
   C4A(R16G16B16A16_USCALED, NONE, C0, C1, C2, C3, USCALED, 16_16_16_16, V, 0),
   F3A(R16G16B16_FLOAT, NONE, C0, C1, C2, xx, FLOAT, 16_16_16, V),
   F3A(R16G16B16_UNORM, NONE, C0, C1, C2, xx, UNORM, 16_16_16, V),
   F3A(R16G16B16_SNORM, NONE, C0, C1, C2, xx, SNORM, 16_16_16, V),
   I3A(R16G16B16_SINT, NONE, C0, C1, C2, xx, SINT, 16_16_16, V),
   I3A(R16G16B16_UINT, NONE, C0, C1, C2, xx, UINT, 16_16_16, V),
   F3A(R16G16B16_SSCALED, NONE, C0, C1, C2, xx, SSCALED, 16_16_16, V),
   F3A(R16G16B16_USCALED, NONE, C0, C1, C2, xx, USCALED, 16_16_16, V),
   F2A(R16G16_SSCALED, NONE, C0, C1, xx, xx, SSCALED, 16_16, V),
   F2A(R16G16_USCALED, NONE, C0, C1, xx, xx, USCALED, 16_16, V),
   F1A(R16_SSCALED, NONE, C0, xx, xx, xx, SSCALED, 16, V),
   F1A(R16_USCALED, NONE, C0, xx, xx, xx, USCALED, 16, V),

   C4A(R8G8B8A8_SSCALED, NONE, C0, C1, C2, C3, SSCALED, 8_8_8_8, V, 0),
   C4A(R8G8B8A8_USCALED, NONE, C0, C1, C2, C3, USCALED, 8_8_8_8, V, 0),
   F3A(R8G8B8_UNORM, NONE, C0, C1, C2, xx, UNORM, 8_8_8, V),
   F3A(R8G8B8_SNORM, NONE, C0, C1, C2, xx, SNORM, 8_8_8, V),
   I2A(R8G8B8_SINT, NONE, C0, C1, C2, xx, SINT, 8_8_8, V),
   I2A(R8G8B8_UINT, NONE, C0, C1, C2, xx, UINT, 8_8_8, V),
   F3A(R8G8B8_SSCALED, NONE, C0, C1, C2, xx, SSCALED, 8_8_8, V),
   F3A(R8G8B8_USCALED, NONE, C0, C1, C2, xx, USCALED, 8_8_8, V),
   F2A(R8G8_SSCALED, NONE, C0, C1, xx, xx, SSCALED, 8_8, V),
   F2A(R8G8_USCALED, NONE, C0, C1, xx, xx, USCALED, 8_8, V),
   F1A(R8_SSCALED, NONE, C0, xx, xx, xx, SSCALED, 8, V),
   F1A(R8_USCALED, NONE, C0, xx, xx, xx, USCALED, 8, V),

   /* FIXED types: not supported natively, converted on VBO push */

   C4B(R32G32B32A32_FIXED, NONE, C0, C1, C2, C3, FLOAT, 32_32_32_32, V),
   F3B(R32G32B32_FIXED, NONE, C0, C1, C2, xx, FLOAT, 32_32_32, V),
   F2B(R32G32_FIXED, NONE, C0, C1, xx, xx, FLOAT, 32_32, V),
   F1B(R32_FIXED, NONE, C0, xx, xx, xx, FLOAT, 32, V),

   C4B(R64G64B64A64_FLOAT, NONE, C0, C1, C2, C3, FLOAT, 32_32_32_32, V),
   F3B(R64G64B64_FLOAT, NONE, C0, C1, C2, xx, FLOAT, 32_32_32, V),
   F2B(R64G64_FLOAT, NONE, C0, C1, xx, xx, FLOAT, 32_32, V),
   F1B(R64_FLOAT, NONE, C0, xx, xx, xx, FLOAT, 32, V),
};

#if 0
const uint8_t nv50_rt_format_map[PIPE_FORMAT_COUNT] =
{
   [PIPE_FORMAT_Z16_UNORM]            = NV50_ZETA_FORMAT_Z16_UNORM,
   [PIPE_FORMAT_Z24X8_UNORM]          = NV50_ZETA_FORMAT_Z24_X8_UNORM,
   [PIPE_FORMAT_Z24_UNORM_S8_UINT]    = NV50_ZETA_FORMAT_Z24_S8_UNORM,
   [PIPE_FORMAT_S8_UINT_Z24_UNORM]    = NV50_ZETA_FORMAT_S8_Z24_UNORM,
   [PIPE_FORMAT_Z32_FLOAT]            = NV50_ZETA_FORMAT_Z32_FLOAT,
   [PIPE_FORMAT_Z32_FLOAT_S8X24_UINT] = NV50_ZETA_FORMAT_Z32_S8_X24_FLOAT,

   [PIPE_FORMAT_R1_UNORM] = NV50_SURFACE_FORMAT_BITMAP,

   [PIPE_FORMAT_R32G32B32A32_FLOAT] = NV50_SURFACE_FORMAT_RGBA32_FLOAT,
   [PIPE_FORMAT_R32G32B32X32_FLOAT] = NV50_SURFACE_FORMAT_RGBX32_FLOAT,
   [PIPE_FORMAT_R32G32B32A32_SINT]  = NV50_SURFACE_FORMAT_RGBA32_SINT,
   [PIPE_FORMAT_R32G32B32X32_SINT]  = NV50_SURFACE_FORMAT_RGBX32_SINT,
   [PIPE_FORMAT_R32G32B32A32_UINT]  = NV50_SURFACE_FORMAT_RGBA32_UINT,
   [PIPE_FORMAT_R32G32B32X32_UINT]  = NV50_SURFACE_FORMAT_RGBX32_UINT,

   [PIPE_FORMAT_R16G16B16A16_FLOAT] = NV50_SURFACE_FORMAT_RGBA16_FLOAT,
   [PIPE_FORMAT_R16G16B16X16_FLOAT] = NV50_SURFACE_FORMAT_RGBX16_FLOAT,
   [PIPE_FORMAT_R16G16B16A16_UNORM] = NV50_SURFACE_FORMAT_RGBA16_UNORM,
   [PIPE_FORMAT_R16G16B16A16_SNORM] = NV50_SURFACE_FORMAT_RGBA16_SNORM,
   [PIPE_FORMAT_R16G16B16A16_SINT]  = NV50_SURFACE_FORMAT_RGBA16_SINT,
   [PIPE_FORMAT_R16G16B16A16_UINT]  = NV50_SURFACE_FORMAT_RGBA16_UINT,

   [PIPE_FORMAT_B8G8R8A8_UNORM] = NV50_SURFACE_FORMAT_BGRA8_UNORM,
   [PIPE_FORMAT_R8G8B8A8_UNORM] = NV50_SURFACE_FORMAT_RGBA8_UNORM,
   [PIPE_FORMAT_B8G8R8X8_UNORM] = NV50_SURFACE_FORMAT_BGRX8_UNORM,
   [PIPE_FORMAT_R8G8B8X8_UNORM] = NV50_SURFACE_FORMAT_RGBX8_UNORM,
   [PIPE_FORMAT_B8G8R8A8_SRGB]  = NV50_SURFACE_FORMAT_BGRA8_SRGB,
   [PIPE_FORMAT_R8G8B8A8_SRGB]  = NV50_SURFACE_FORMAT_RGBA8_SRGB,
   [PIPE_FORMAT_B8G8R8X8_SRGB]  = NV50_SURFACE_FORMAT_BGRX8_SRGB,
   [PIPE_FORMAT_R8G8B8X8_SRGB]  = NV50_SURFACE_FORMAT_RGBX8_SRGB,
   [PIPE_FORMAT_R8G8B8A8_SNORM] = NV50_SURFACE_FORMAT_RGBA8_SNORM,
   [PIPE_FORMAT_R8G8B8A8_SINT]  = NV50_SURFACE_FORMAT_RGBA8_SINT,
   [PIPE_FORMAT_R8G8B8A8_UINT]  = NV50_SURFACE_FORMAT_RGBA8_UINT,

   [PIPE_FORMAT_R11G11B10_FLOAT] = NV50_SURFACE_FORMAT_R11G11B10_FLOAT,

   [PIPE_FORMAT_B10G10R10A2_UNORM] = NV50_SURFACE_FORMAT_BGR10_A2_UNORM,
   [PIPE_FORMAT_R10G10B10A2_UNORM] = NV50_SURFACE_FORMAT_RGB10_A2_UNORM,
   [PIPE_FORMAT_R10G10B10A2_UINT]  = NV50_SURFACE_FORMAT_RGB10_A2_UINT,

   [PIPE_FORMAT_B5G6R5_UNORM] = NV50_SURFACE_FORMAT_B5G6R5_UNORM,

   [PIPE_FORMAT_B5G5R5A1_UNORM] = NV50_SURFACE_FORMAT_BGR5_A1_UNORM,
   [PIPE_FORMAT_B5G5R5X1_UNORM] = NV50_SURFACE_FORMAT_BGR5_X1_UNORM,

   [PIPE_FORMAT_R32G32_FLOAT] = NV50_SURFACE_FORMAT_RG32_FLOAT,
   [PIPE_FORMAT_R32G32_SINT]  = NV50_SURFACE_FORMAT_RG32_SINT,
   [PIPE_FORMAT_R32G32_UINT]  = NV50_SURFACE_FORMAT_RG32_UINT,

   [PIPE_FORMAT_R16G16_FLOAT] = NV50_SURFACE_FORMAT_RG16_FLOAT,
   [PIPE_FORMAT_R16G16_UNORM] = NV50_SURFACE_FORMAT_RG16_UNORM,
   [PIPE_FORMAT_R16G16_SNORM] = NV50_SURFACE_FORMAT_RG16_SNORM,
   [PIPE_FORMAT_R16G16_SINT]  = NV50_SURFACE_FORMAT_RG16_SINT,
   [PIPE_FORMAT_R16G16_UINT]  = NV50_SURFACE_FORMAT_RG16_UINT,

   [PIPE_FORMAT_R8G8_UNORM] = NV50_SURFACE_FORMAT_RG8_UNORM,
   [PIPE_FORMAT_R8G8_SNORM] = NV50_SURFACE_FORMAT_RG8_SNORM,
   [PIPE_FORMAT_R8G8_SINT]  = NV50_SURFACE_FORMAT_RG8_SINT,
   [PIPE_FORMAT_R8G8_UINT]  = NV50_SURFACE_FORMAT_RG8_UINT,

   [PIPE_FORMAT_R32_FLOAT] = NV50_SURFACE_FORMAT_R32_FLOAT,
   [PIPE_FORMAT_R32_SINT]  = NV50_SURFACE_FORMAT_R32_SINT,
   [PIPE_FORMAT_R32_UINT]  = NV50_SURFACE_FORMAT_R32_UINT,

   [PIPE_FORMAT_R16_FLOAT] = NV50_SURFACE_FORMAT_R16_FLOAT,
   [PIPE_FORMAT_R16_UNORM] = NV50_SURFACE_FORMAT_R16_UNORM,
   [PIPE_FORMAT_R16_SNORM] = NV50_SURFACE_FORMAT_R16_SNORM,
   [PIPE_FORMAT_R16_SINT]  = NV50_SURFACE_FORMAT_R16_SINT,
   [PIPE_FORMAT_R16_UINT]  = NV50_SURFACE_FORMAT_R16_UINT,

   [PIPE_FORMAT_R8_UNORM] = NV50_SURFACE_FORMAT_R8_UNORM,
   [PIPE_FORMAT_R8_SNORM] = NV50_SURFACE_FORMAT_R8_SNORM,
   [PIPE_FORMAT_R8_SINT]  = NV50_SURFACE_FORMAT_R8_SINT,
   [PIPE_FORMAT_R8_UINT]  = NV50_SURFACE_FORMAT_R8_UINT,

   [PIPE_FORMAT_A8_UNORM] = NV50_SURFACE_FORMAT_A8_UNORM
};
#endif
