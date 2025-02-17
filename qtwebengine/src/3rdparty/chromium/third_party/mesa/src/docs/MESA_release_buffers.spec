Name

    MESA_release_buffers

Name Strings

    GLX_MESA_release_buffers

Contact

    Brian Paul (brian.paul 'at' tungstengraphics.com)

Status

    Shipping since Mesa 2.0 in October, 1996.

Version

    Last Modified Date:  8 June 2000

Number

    217

Dependencies

    OpenGL 1.0 or later is required.
    GLX 1.0 or later is required.

Overview

    Mesa's implementation of GLX is entirely implemented on the client side.
    Therefore, Mesa cannot immediately detect when an X window or pixmap is
    destroyed in order to free any ancillary data associated with the window
    or pixmap.

    The glxMesaReleaseBuffers() function can be used to explicitly indicate
    when the back color buffer, depth buffer, stencil buffer, and/or accumu-
    lation buffer associated with a drawable can be freed.

IP Status

    Open-source; freely implementable.

Issues

    None.

New Procedures and Functions

    Bool glXReleaseBuffersMESA( Display *dpy, GLXDrawable d );

New Tokens

    None.

Additions to Chapter 3 of the GLX 1.3 Specification (Functions and Errors)

    The function

	Bool glXReleaseBuffersMESA( Display *dpy, GLXDrawable d );

    causes all software ancillary buffers (back buffer, depth, stencil,
    accum, etc) associated with the named drawable to be immediately
    deallocated.  True is returned if <d> is a valid Mesa GLX drawable,
    else False is returned.  After calling glXReleaseBuffersMESA, the
    drawable should no longer be used for GL rendering.  Results of
    attempting to do so are undefined.


GLX Protocol

    None, since this is a client-side operation.

Errors

    None.

New State

    None.

Revision History

    8 June 2000 - initial specification
