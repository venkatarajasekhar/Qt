
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkRefCnt.h"

#ifndef SkWGL_DEFINED
#define SkWGL_DEFINED

/**
 * Working with WGL extensions can be a pain. Among the reasons is that You must
 * have a GL context to get the proc addresses, but you want to use the procs to
 * create a context in the first place. So you have to create a dummy GL ctx to
 * get the proc addresses.
 *
 * This file helps by providing SkCreateWGLInterface(). It returns a struct of
 * function pointers that it initializes. It also has a helper function to query
 * for WGL extensions. It handles the fact that wglGetExtensionsString is itself
 * an extension.
 */

#if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
    #define SK_LOCAL_LEAN_AND_MEAN
#endif
#include <windows.h>
#if defined(SK_LOCAL_LEAN_AND_MEAN)
    #undef WIN32_LEAN_AND_MEAN
    #undef SK_LOCAL_LEAN_AND_MEAN
#endif

#define SK_WGL_DRAW_TO_WINDOW                       0x2001
#define SK_WGL_ACCELERATION                         0x2003
#define SK_WGL_SUPPORT_OPENGL                       0x2010
#define SK_WGL_DOUBLE_BUFFER                        0x2011
#define SK_WGL_COLOR_BITS                           0x2014
#define SK_WGL_ALPHA_BITS                           0x201B
#define SK_WGL_STENCIL_BITS                         0x2023
#define SK_WGL_FULL_ACCELERATION                    0x2027
#define SK_WGL_SAMPLE_BUFFERS                       0x2041
#define SK_WGL_SAMPLES                              0x2042
#define SK_WGL_CONTEXT_MAJOR_VERSION                0x2091
#define SK_WGL_CONTEXT_MINOR_VERSION                0x2092
#define SK_WGL_CONTEXT_LAYER_PLANE                  0x2093
#define SK_WGL_CONTEXT_FLAGS                        0x2094
#define SK_WGL_CONTEXT_PROFILE_MASK                 0x9126
#define SK_WGL_CONTEXT_DEBUG_BIT                    0x0001
#define SK_WGL_CONTEXT_FORWARD_COMPATIBLE_BIT       0x0002
#define SK_WGL_CONTEXT_CORE_PROFILE_BIT             0x00000001
#define SK_WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT    0x00000002
#define SK_WGL_CONTEXT_ES2_PROFILE_BIT              0x00000004
#define SK_ERROR_INVALID_VERSION                    0x2095
#define SK_ERROR_INVALID_PROFILE                    0x2096

class SkWGLExtensions {
public:
    SkWGLExtensions();
    /**
     * Determines if an extensions is available for a given DC.
     * WGL_extensions_string is considered a prerequisite for all other
     * extensions. It is necessary to check this before calling other class
     * functions.
     */
    bool hasExtension(HDC dc, const char* ext) const;

    const char* getExtensionsString(HDC hdc) const;
    BOOL choosePixelFormat(HDC hdc, const int*, const FLOAT*, UINT, int*, UINT*) const;
    BOOL getPixelFormatAttribiv(HDC, int, int, UINT, const int*, int*) const;
    BOOL getPixelFormatAttribfv(HDC hdc, int, int, UINT, const int*, FLOAT*) const;
    HGLRC createContextAttribs(HDC, HGLRC, const int *) const;

    /**
     * WGL doesn't have precise rules for the ordering of formats returned
     * by wglChoosePixelFormat. This function helps choose among the set of
     * formats returned by wglChoosePixelFormat. The rules in decreasing
     * priority are:
     *     * Choose formats with the smallest sample count that is >=
     *       desiredSampleCount (or the largest sample count if all formats have
     *       fewer samples than desiredSampleCount.)
     *     * Choose formats with the fewest color samples when coverage sampling
     *       is available.
     *     * If the above rules leave multiple formats, choose the one that
     *       appears first in the formats array parameter.
     */
    int selectFormat(const int formats[],
                     int formatCount,
                     HDC dc,
                     int desiredSampleCount);
private:
    typedef const char* (WINAPI *GetExtensionsStringProc)(HDC hdc);
    typedef BOOL (WINAPI *ChoosePixelFormatProc)(HDC hdc, const int *, const FLOAT *, UINT, int *, UINT *);
    typedef BOOL (WINAPI *GetPixelFormatAttribivProc)(HDC, int, int, UINT, const int*, int*);
    typedef BOOL (WINAPI *GetPixelFormatAttribfvProc)(HDC hdc, int, int, UINT, const int*, FLOAT*);
    typedef HGLRC (WINAPI *CreateContextAttribsProc)(HDC hDC, HGLRC, const int *);

    GetExtensionsStringProc fGetExtensionsString;
    ChoosePixelFormatProc fChoosePixelFormat;
    GetPixelFormatAttribfvProc fGetPixelFormatAttribfv;
    GetPixelFormatAttribivProc fGetPixelFormatAttribiv;
    CreateContextAttribsProc fCreateContextAttribs;
};

/**
 * Helper to create an OpenGL context for a DC using WGL. Configs with a sample count >= to
 * msaaSampleCount are preferred but if none is available then a context with a lower sample count
 * (including non-MSAA) will be created. If preferCoreProfile is true but a core profile cannot be
 * created then a compatible profile context will be created.
 */
HGLRC SkCreateWGLContext(HDC dc, int msaaSampleCount, bool preferCoreProfile);

#endif
