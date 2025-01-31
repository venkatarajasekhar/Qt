Name

    MESA_swap_control

Name Strings

    GLX_MESA_swap_control

Contact

    Ian Romanick, IBM, idr at us.ibm.com

Status

    Deployed in DRI drivers post-XFree86 4.3.

Version

    Date: 5/1/2003   Revision: 1.1

Number

    ???

Dependencies

    None

    Based on GLX_SGI_swap_control version 1.9 and WGL_EXT_swap_control
    version 1.5.

Overview

    This extension allows an application to specify a minimum periodicity
    of color buffer swaps, measured in video frame periods.

Issues

    * Should implementations that export GLX_MESA_swap_control also export
      GL_EXT_swap_control for compatibility with WGL_EXT_swap_control?

    UNRESOLVED.

New Procedures and Functions

    int glXSwapIntervalMESA(unsigned int interval)
    int glXGetSwapIntervalMESA(void)

New Tokens

    None

Additions to Chapter 2 of the 1.4 GL Specification (OpenGL Operation)

    None

Additions to Chapter 3 of the 1.4 GL Specification (Rasterization)

    None

Additions to Chapter 4 of the 1.4 GL Specification (Per-Fragment Operations
and the Framebuffer)

    None

Additions to Chapter 5 of the 1.4 GL Specification (Special Functions)

    None

Additions to Chapter 6 of the 1.4 GL Specification (State and State Requests)

    None

Additions to the GLX 1.3 Specification

    [Add the following to Section 3.3.10 of the GLX Specification (Double
     Buffering)]

    glXSwapIntervalMESA specifies the minimum number of video frame periods
    per buffer swap.  (e.g. a value of two means that the color buffers
    will be swapped at most every other video frame.)  A return value
    of zero indicates success; otherwise an error occurred.  The interval
    takes effect when glXSwapBuffers is first called subsequent to the
    glXSwapIntervalMESA call.

    A video frame period is the time required by the monitor to display a 
    full frame of video data.  In the case of an interlaced monitor,
    this is typically the time required to display both the even and odd 
    fields of a frame of video data.

    If <interval> is set to a value of 0, buffer swaps are not synchro-
    nized to a video frame.  The <interval> value is silently clamped to
    the maximum implementation-dependent value supported before being
    stored.

    The swap interval is not part of the render context state.  It cannot
    be pushed or popped.  The current swap interval for the window
    associated with the current context can be obtained by calling
    glXGetSwapIntervalMESA.  The default swap interval is 0.

    On XFree86, setting the environment variable LIBGL_THROTTLE_REFRESH sets
    the swap interval to 1.

Errors

    glXSwapIntervalMESA returns GLX_BAD_CONTEXT if there is no current
    GLXContext or if the current context is not a direct rendering context.

GLX Protocol

    None.  This extension only extends to direct rendering contexts.

New State

    Get Value		Get Command	Type	    Initial Value
    ---------		-----------	----	    -------------
    [swap interval]	GetSwapInterval	Z+	    0

New Implementation Dependent State

    None


Revision History

    1.1,  5/1/03   Added the issues section and contact information.
    	  	   Changed the default swap interval to 0.
    1.0,  3/17/03  Initial version based on GLX_SGI_swap_control and
                   WGL_EXT_swap_control.
