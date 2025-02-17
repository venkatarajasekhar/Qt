Name

    MESA_pack_invert

Name Strings

    GL_MESA_pack_invert

Contact

    Brian Paul, Tungsten Graphics, Inc. (brian.paul 'at' tungstengraphics.com)
    Keith Whitwell, Tungsten Graphics, Inc.  (keith 'at' tungstengraphics.com)

Status

    Shipping (Mesa 4.0.4 and later)

Version

    1.0

Number

    TBD

Dependencies

    OpenGL 1.0 or later is required
    This extensions is written against the OpenGL 1.4 Specification.

Overview

    This extension adds a new pixel storage parameter to indicate that
    images are to be packed in top-to-bottom order instead of OpenGL's
    conventional bottom-to-top order.  Only pixel packing can be
    inverted (i.e. for glReadPixels, glGetTexImage, glGetConvolutionFilter,
    etc).

    Almost all known image file formats store images in top-to-bottom
    order.  As it is, OpenGL reads images from the frame buffer in
    bottom-to-top order.  Thus, images usually have to be inverted before
    writing them to a file with image I/O libraries.  This extension
    allows images to be read such that inverting isn't needed.

IP Status

    None

Issues

    1. Should we also define UNPACK_INVERT_MESA for glDrawPixels, etc?

    Resolved:  No, we're only concerned with pixel packing.  There are other
    solutions for inverting images when using glDrawPixels (negative Y pixel
    zoom) or glTexImage (invert the vertex T coordinates).  It would be easy
    enough to define a complementary extension for pixel packing in the
    future if needed.

New Procedures and Functions

    None

New Tokens

    Accepted by the <pname> parameter of PixelStorei and PixelStoref
    and the <pname> parameter of GetIntegerv, GetFloatv, GetDoublev
    and GetBooleanv:

        PACK_INVERT_MESA                   0x8758

Additions to Chapter 2 of the OpenGL 1.4 Specification (OpenGL Operation)

    None

Additions to Chapter 3 of the OpenGL 1.4 Specification (Rasterization)

    None

Additions to Chapter 4 of the OpenGL 1.4 Specification (Per-Fragment
Operations and the Frame Buffer)

    Add the following entry to table 4.4 (PixelStore parameters) on page 182:

    Parameter Name       Type    Initial Value    Valid Range
    ---------------------------------------------------------
    PACK_INVERT_MESA     boolean     FALSE        TRUE/FALSE

    In the section labeled "Placement in Client Memory" on page 184
    insert the following text into the paragraph before the sentence
    that starts with "If the format is RED, GREEN, BLUE...":

    "The parameter PACK_INVERT_MESA controls whether the image is packed
     in bottom-to-top order (the default) or top-to-bottom order.  Equation
     3.8 is modified as follows:

     ... the first element of the Nth row is indicated by

         p + Nk,                if PACK_INVERT_MESA is false
         p + k * (H - 1) - Nk,  if PACK_INVERT_MESA is true, where H is the
                                image height
    "

Additions to Chapter 5 of the OpenGL 1.4 Specification (Special Functions)

    None

Additions to Chapter 6 of the OpenGL 1.4 Specification (State and
State Requests)

    None

Additions to Appendix A of the OpenGL 1.4 Specification (Invariance)

    None

Additions to the AGL/GLX/WGL Specifications

    None

GLX Protocol

    None

Errors

    None

New State

    Add the following entry to table 6.20 (Pixels) on page 235:

    Get Value         Type     Get Cmd    Initial Value  Description                Sec    Attribute
    --------------------------------------------------------------------------------------------------
    PACK_INVERT_MESA  boolean  GetBoolean  FALSE         Value of PACK_INVERT_MESA  4.3.2  pixel-store

Revision History

    21 September 2002 - Initial draft
