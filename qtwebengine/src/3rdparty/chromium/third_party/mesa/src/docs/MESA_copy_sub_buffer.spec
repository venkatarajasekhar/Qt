Name

    MESA_copy_sub_buffer

Name Strings

    GLX_MESA_copy_sub_buffer

Contact

    Brian Paul (brian.paul 'at' tungstengraphics.com)

Status

    Shipping since Mesa 2.6 in February, 1998.

Version

    Last Modified Date:  12 January 2009

Number

    215

Dependencies

    OpenGL 1.0 or later is required.
    GLX 1.0 or later is required.

Overview

    The glxCopySubBufferMESA() function copies a rectangular region
    of the back color buffer to the front color buffer.  This can be
    used to quickly repaint 3D windows in response to expose events
    when the back color buffer cannot be damaged by other windows.

IP Status

    Open-source; freely implementable.

Issues

    None.

New Procedures and Functions

    void glXCopySubBufferMESA( Display *dpy, GLXDrawable drawable,
			       int x, int y, int width, int height );

New Tokens

    None.

Additions to Chapter 3 of the GLX 1.3 Specification (Functions and Errors)

    Add to section 3.3.10 Double Buffering:

    The function

	 void glXCopySubBufferMESA( Display *dpy, GLXDrawable drawable,
				    int x, int y, int width, int height );

    may be used to copy a rectangular region of the back color buffer to
    the front color buffer.  This can be used to quickly repaint 3D windows
    in response to expose events when the back color buffer cannot be
    damaged by other windows.

    <x> and <y> indicates the lower-left corner of the region to copy and
    <width> and <height> indicate the size in pixels.  Coordinate (0,0)
    corresponds to the lower-left pixel of the window, like glReadPixels.

    If dpy and drawable are the display and drawable for the calling
    thread's current context, glXCopySubBufferMESA performs an
    implicit glFlush before it returns.  Subsequent OpenGL commands
    may be issued immediately after calling glXCopySubBufferMESA, but
    are not executed until the copy is completed. 

GLX Protocol

    None at this time.  The extension is implemented in terms of ordinary
    Xlib protocol inside of Mesa.

Errors

    None.

New State

    None.

Revision History

    12 January 2009 Ian Romanick - Added language about implicit flush
                                   and command completion.
    8 June 2000     Brian Paul   - initial specification

