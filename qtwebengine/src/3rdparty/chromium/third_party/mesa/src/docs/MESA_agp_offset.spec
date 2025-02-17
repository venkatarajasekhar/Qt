Name

    MESA_agp_offset

Name Strings

    GLX_MESA_agp_offset

Contact

    Brian Paul, Tungsten Graphics, Inc. (brian.paul 'at' tungstengraphics.com)
    Keith Whitwell, Tungsten Graphics, Inc.  (keith 'at' tungstengraphics.com)

Status

    Shipping (Mesa 4.0.4 and later.  Only implemented in particular
    XFree86/DRI drivers.)

Version

    1.0

Number

    TBD

Dependencies

    OpenGL 1.0 or later is required
    GLX_NV_vertex_array_range is required.
    This extensions is written against the OpenGL 1.4 Specification.

Overview

    This extensions provides a way to convert pointers in an AGP memory
    region into byte offsets into the AGP aperture.
    Note, this extension depends on GLX_NV_vertex_array_range, for which
    no real specification exists.  See GL_NV_vertex_array_range for more
    information.

IP Status

    None

Issues

    None

New Procedures and Functions

    unsigned int glXGetAGPOffsetMESA( const void *pointer )

New Tokens

    None

Additions to the OpenGL 1.4 Specification

    None

Additions to Chapter 3 the GLX 1.4 Specification (Functions and Errors)

    Add a new section, 3.6 as follows:

    3.6 AGP Memory Access

    On "PC" computers, AGP memory can be allocated with glXAllocateMemoryNV
    and freed with glXFreeMemoryNV.  Sometimes it's useful to know where a
    block of AGP memory is located with respect to the start of the AGP
    aperture.  The function

        GLuint glXGetAGPOffsetMESA( const GLvoid *pointer )

    Returns the offset of the given memory block from the start of AGP
    memory in basic machine units (i.e. bytes).  If pointer is invalid
    the value ~0 will be returned.

GLX Protocol

    None.  This is a client side-only extension.

Errors

    glXGetAGPOffsetMESA will return ~0 if the pointer does not point to
    an AGP memory region.

New State

    None

Revision History

    20 September 2002 - Initial draft
    2 October 2002 - finished GLX chapter 3 additions
    27 July 2004 - use unsigned int instead of GLuint, void instead of GLvoid
