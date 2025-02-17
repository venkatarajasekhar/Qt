Name

    MESA_pixmap_colormap

Name Strings

    GLX_MESA_pixmap_colormap

Contact

    Brian Paul (brian.paul 'at' tungstengraphics.com)

Status

    Shipping since Mesa 1.2.8 in May, 1996.

Version

    Last Modified Date:  8 June 2000

Number

    216

Dependencies

    OpenGL 1.0 or later is required.
    GLX 1.0 or later is required.

Overview

    Since Mesa allows RGB rendering into drawables with PseudoColor,
    StaticColor, GrayScale and StaticGray visuals, Mesa needs a colormap
    in order to compute pixel values during rendering.

    The colormap associated with a window can be queried with normal
    Xlib functions but there is no colormap associated with pixmaps.

    The glXCreateGLXPixmapMESA function is an alternative to glXCreateGLXPixmap
    which allows specification of a colormap.

IP Status

    Open-source; freely implementable.

Issues

    None.

New Procedures and Functions

    GLXPixmap glXCreateGLXPixmapMESA( Display *dpy, XVisualInfo *visual,
				      Pixmap pixmap, Colormap cmap );

New Tokens

    None.

Additions to Chapter 3 of the GLX 1.3 Specification (Functions and Errors)

    Add to section 3.4.2 Off Screen Rendering

    The Mesa implementation of GLX allows RGB rendering into X windows and
    pixmaps of any visual class, not just TrueColor or DirectColor.  In order
    to compute pixel values from RGB values Mesa requires a colormap.

    The function

	GLXPixmap glXCreateGLXPixmapMESA( Display *dpy, XVisualInfo *visual,
					  Pixmap pixmap, Colormap cmap );

    allows one to create a GLXPixmap with a specific colormap.  The image
    rendered into the pixmap may then be copied to a window (which uses the
    same colormap and visual) with the expected results.

GLX Protocol

    None since this is a client-side extension.

Errors

    None.

New State

    None.

Revision History

    8 June 2000 - initial specification
