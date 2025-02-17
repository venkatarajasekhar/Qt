Name

    MESA_resize_buffers

Name Strings

    GL_MESA_resize_buffers

Contact

    Brian Paul (brian.paul 'at' tungstengraphics.com)

Status

    Shipping (since Mesa version 2.2)

Version


Number

    196

Dependencies

    Mesa 2.2 or later is required.

Overview

    Mesa is often used as a client library with no integration with
    the computer's window system (an X server, for example).  And since
    Mesa does not have an event loop nor window system callbacks, it
    cannot properly respond to window system events.  In particular,
    Mesa cannot automatically detect when a window has been resized.

    Mesa's glViewport command queries the current window size and updates
    its internal data structors accordingly.  This normally works fine
    since most applications call glViewport in response to window size
    changes.

    In some situations, however, the application may not call glViewport
    when a window size changes but would still like Mesa to adjust to
    the new window size.  This extension exports a new function to solve
    this problem.

New Procedures and Functions

    void glResizeBuffersMESA( void )

New Tokens

    none

Additions to the OpenGL Specification (no particular section)

    The glResizeBuffersMESA command may be called when the client
    determines that a window has been resized.  Calling
    glResizeBuffersMESA causes Mesa to query the current window size
    and adjust its internal data structures.  This may include
    reallocating depth, stencil, alpha and accumulation buffers.

Additions to the AGL/GLX/WGL Specifications

    None

Errors

    INVALID_OPERATION is generated if glResizeBuffersMESA is called between
    Begin and End.

New State

    None.

New Implementation Dependent State

    None.

Revision History

  * Revision 1.0 - Initial specification
