Name

    MESA_window_pos

Name Strings

    GL_MESA_window_pos

Contact

    Brian Paul, brian.paul 'at' tungstengraphics.com

Status

    Shipping (since Mesa version 1.2.8)

Version


Number

    197

Dependencies

    OpenGL 1.0 is required.
    The extension is written against the OpenGL 1.2 Specification

Overview

    In order to set the current raster position to a specific window
    coordinate with the RasterPos command, the modelview matrix, projection
    matrix and viewport must be set very carefully.  Furthermore, if the
    desired window coordinate is outside of the window's bounds one must
    rely on a subtle side-effect of the Bitmap command in order to circumvent
    frustum clipping.

    This extension provides a set of functions to directly set the
    current raster position, bypassing the modelview matrix, the
    projection matrix and the viewport to window mapping.  Furthermore,
    clip testing is not performed.

    This greatly simplifies the process of setting the current raster
    position to a specific window coordinate prior to calling DrawPixels,
    CopyPixels or Bitmap.

New Procedures and Functions

    void WindowPos2dMESA(double x, double y)
    void WindowPos2fMESA(float x, float y)
    void WindowPos2iMESA(int x, int y)
    void WindowPos2sMESA(short x, short y)
    void WindowPos2ivMESA(const int *p)
    void WindowPos2svMESA(const short *p)
    void WindowPos2fvMESA(const float *p)
    void WindowPos2dvMESA(const double *p)
    void WindowPos3iMESA(int x, int y, int z)
    void WindowPos3sMESA(short x, short y, short z)
    void WindowPos3fMESA(float x, float y, float z)
    void WindowPos3dMESA(double x, double y, double z)
    void WindowPos3ivMESA(const int *p)
    void WindowPos3svMESA(const short *p)
    void WindowPos3fvMESA(const float *p)
    void WindowPos3dvMESA(const double *p)
    void WindowPos4iMESA(int x, int y, int z, int w)
    void WindowPos4sMESA(short x, short y, short z, short w)
    void WindowPos4fMESA(float x, float y, float z, float w)
    void WindowPos4dMESA(double x, double y, double z, double )
    void WindowPos4ivMESA(const int *p)
    void WindowPos4svMESA(const short *p)
    void WindowPos4fvMESA(const float *p)
    void WindowPos4dvMESA(const double *p)

New Tokens

    none

Additions to Chapter 2 of the OpenGL 1.2 Specification (OpenGL Operation)

  - (2.12, p. 41) Insert after third paragraph:

      Alternately, the current raster position may be set by one of the
      WindowPosMESA commands:

         void WindowPos{234}{sidf}MESA( T coords );
         void WindowPos{234}{sidf}vMESA( T coords );

      WindosPos4MESA takes four values indicating x, y, z, and w.
      WindowPos3MESA (or WindowPos2MESA) is analaguos, but sets only
      x, y, and z with w implicitly set to 1 (or only x and y with z
      implicitly set to 0 and w implicitly set to 1).

      WindowPosMESA operates like RasterPos except that the current modelview
      matrix, projection matrix and viewport parameters are ignored and the
      clip test operation always passes.  The current raster position values
      are directly set to the parameters passed to WindowPosMESA.  The current
      color, color index and texture coordinate update the current raster
      position's associated data.

Additions to the AGL/GLX/WGL Specifications

    None

GLX Protocol

    Not specified at this time.  However, a protocol message very similar
    to that of RasterPos is expected.

Errors

    INVALID_OPERATION is generated if WindowPosMESA is called between
    Begin and End.

New State

    None.

New Implementation Dependent State

    None.

Revision History

  * Revision 1.0 - Initial specification
  * Revision 1.1 - Minor clean-up  (7 Jan 2000, Brian Paul)

