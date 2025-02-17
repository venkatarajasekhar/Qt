Name

    MESA_drm_image

Name Strings

    EGL_MESA_drm_image

Contact

    Kristian Høgsberg <krh@bitplanet.net>

Status

    Proposal

Version

    Version 2, August 25, 2010

Number

    EGL Extension #not assigned

Dependencies

    Requires EGL 1.4 or later.  This extension is written against the
    wording of the EGL 1.4 specification.

    EGL_KHR_base_image is required.

Overview

    This extension provides entry points for integrating EGLImage with the
    Linux DRM mode setting and memory management drivers.  The extension
    lets applications create EGLImages without a client API resource and
    lets the application get the DRM buffer handles.

IP Status

    Open-source; freely implementable.

New Procedures and Functions

    EGLImageKHR eglCreateDRMImageMESA(EGLDisplay dpy,
                                      const EGLint *attrib_list);

    EGLBoolean eglExportDRMImageMESA(EGLDisplay dpy,
                                     EGLImageKHR image,
                                     EGLint *name,
				     EGLint *handle,
				     EGLint *stride);

New Tokens

    Accepted in the <attrib_list> parameter of eglCreateDRMImageMESA:

        EGL_DRM_BUFFER_FORMAT_MESA		0x31D0
        EGL_DRM_BUFFER_USE_MESA			0x31D1

    Accepted as values for the EGL_IMAGE_FORMAT_MESA attribute:

        EGL_DRM_BUFFER_FORMAT_ARGB32_MESA	0x31D2

    Bits accepted in EGL_DRM_BUFFER_USE_MESA:

        EGL_DRM_BUFFER_USE_SCANOUT_MESA		0x0001
        EGL_DRM_BUFFER_USE_SHARE_MESA		0x0002
        EGL_DRM_BUFFER_USE_CURSOR_MESA		0x0004

    Accepted in the <target> parameter of eglCreateImageKHR:

        EGL_DRM_BUFFER_MESA			0x31D3

    Use when importing drm buffer:

        EGL_DRM_BUFFER_STRIDE_MESA		0x31D4
        EGL_DRM_BUFFER_FORMAT_MESA		0x31D0

Additions to the EGL 1.4 Specification:

    To create a DRM EGLImage, call

        EGLImageKHR eglCreateDRMImageMESA(EGLDisplay dpy,
                                          const EGLint *attrib_list);

    In the attribute list, pass EGL_WIDTH, EGL_HEIGHT and format and
    use in the attrib list using EGL_DRM_BUFFER_FORMAT_MESA and
    EGL_DRM_BUFFER_USE_MESA.  The only format specified by this
    extension is EGL_DRM_BUFFER_FORMAT_ARGB32_MESA, where each pixel
    is a CPU-endian, 32-bit quantity, with alpha in the upper 8 bits,
    then red, then green, then blue.  The bit values accepted by
    EGL_DRM_BUFFER_USE_MESA are EGL_DRM_BUFFER_USE_SCANOUT_MESA,
    EGL_DRM_BUFFER_USE_SHARE_MESA and EGL_DRM_BUFFER_USE_CURSOR_MESA.
    EGL_DRM_BUFFER_USE_SCANOUT_MESA requests that the created EGLImage
    should be usable as a scanout buffer with the DRM kernel
    modesetting API.  EGL_DRM_BUFFER_USE_SHARE_MESA requests that the
    EGLImage can be shared with other processes by passing the
    underlying DRM buffer name.  EGL_DRM_BUFFER_USE_CURSOR_MESA
    requests that the image must be usable as a cursor with KMS.  When
    EGL_DRM_BUFFER_USE_CURSOR_MESA is set, width and height must both
    be 64.

    To create a process local handle or a global DRM name for a
    buffer, call

        EGLBoolean eglExportDRMImageMESA(EGLDisplay dpy,
                                         EGLImageKHR image,
                                         EGLint *name,
                                         EGLint *handle,
                                         EGLint *stride);

    If <name> is non-NULL, a global name is assigned to the image and
    written to <name>, the handle (local to the DRM file descriptor,
    for use with DRM kernel modesetting API) is written to <handle> if
    non-NULL and the stride (in bytes) is written to <stride>, if
    non-NULL.

    Import a shared buffer by calling eglCreateImageKHR with
    EGL_DRM_BUFFER_MESA as the target, using EGL_WIDTH, EGL_HEIGHT,
    EGL_DRM_BUFFER_FORMAT_MESA, EGL_DRM_BUFFER_STRIDE_MESA
    in the attrib list.

Issues

    1.  Why don't we use eglCreateImageKHR with a target that
        indicates that we want to create an EGLImage from scratch?

        RESOLVED: The eglCreateImageKHR entry point is reserved for
        creating an EGLImage from an already existing client API
        resource.  This is fine when we're creating the EGLImage from
        an existing DRM buffer name, it doesn't seem right to overload
        the function to also allocate the underlying resource.

    2.  Why don't we use an eglQueryImageMESA type functions for
        querying the DRM EGLImage attributes (name, handle, and stride)?

        RESOLVED: The eglQueryImage function has been proposed often,
        but it goes against the EGLImage design.  EGLImages are opaque
        handles to a 2D array of pixels, which can be passed between
        client APIs.  By referencing an EGLImage in a client API, the
        EGLImage target (a texture, a renderbuffer or such) can be
        used to query the attributes of the EGLImage.  We don't have a
        full client API for creating and querying DRM buffers, though,
        so we use a new EGL extension entry point instead.

Revision History

    Version 1, June 3, 2010
        Initial draft (Kristian Høgsberg)
    Version 2, August 25, 2010
        Flesh out the extension a bit, add final EGL tokens, capture
        some of the original discussion in the issues section.
