#ifndef WAYLAND_DRM_H
#define WAYLAND_DRM_H

#include <wayland-server.h>

#ifndef WL_DRM_FORMAT_ENUM
#define WL_DRM_FORMAT_ENUM
enum wl_drm_format {
	WL_DRM_FORMAT_C8 = 0x20203843,
	WL_DRM_FORMAT_RGB332 = 0x38424752,
	WL_DRM_FORMAT_BGR233 = 0x38524742,
	WL_DRM_FORMAT_XRGB4444 = 0x32315258,
	WL_DRM_FORMAT_XBGR4444 = 0x32314258,
	WL_DRM_FORMAT_RGBX4444 = 0x32315852,
	WL_DRM_FORMAT_BGRX4444 = 0x32315842,
	WL_DRM_FORMAT_ARGB4444 = 0x32315241,
	WL_DRM_FORMAT_ABGR4444 = 0x32314241,
	WL_DRM_FORMAT_RGBA4444 = 0x32314152,
	WL_DRM_FORMAT_BGRA4444 = 0x32314142,
	WL_DRM_FORMAT_XRGB1555 = 0x35315258,
	WL_DRM_FORMAT_XBGR1555 = 0x35314258,
	WL_DRM_FORMAT_RGBX5551 = 0x35315852,
	WL_DRM_FORMAT_BGRX5551 = 0x35315842,
	WL_DRM_FORMAT_ARGB1555 = 0x35315241,
	WL_DRM_FORMAT_ABGR1555 = 0x35314241,
	WL_DRM_FORMAT_RGBA5551 = 0x35314152,
	WL_DRM_FORMAT_BGRA5551 = 0x35314142,
	WL_DRM_FORMAT_RGB565 = 0x36314752,
	WL_DRM_FORMAT_BGR565 = 0x36314742,
	WL_DRM_FORMAT_RGB888 = 0x34324752,
	WL_DRM_FORMAT_BGR888 = 0x34324742,
	WL_DRM_FORMAT_XRGB8888 = 0x34325258,
	WL_DRM_FORMAT_XBGR8888 = 0x34324258,
	WL_DRM_FORMAT_RGBX8888 = 0x34325852,
	WL_DRM_FORMAT_BGRX8888 = 0x34325842,
	WL_DRM_FORMAT_ARGB8888 = 0x34325241,
	WL_DRM_FORMAT_ABGR8888 = 0x34324241,
	WL_DRM_FORMAT_RGBA8888 = 0x34324152,
	WL_DRM_FORMAT_BGRA8888 = 0x34324142,
	WL_DRM_FORMAT_XRGB2101010 = 0x30335258,
	WL_DRM_FORMAT_XBGR2101010 = 0x30334258,
	WL_DRM_FORMAT_RGBX1010102 = 0x30335852,
	WL_DRM_FORMAT_BGRX1010102 = 0x30335842,
	WL_DRM_FORMAT_ARGB2101010 = 0x30335241,
	WL_DRM_FORMAT_ABGR2101010 = 0x30334241,
	WL_DRM_FORMAT_RGBA1010102 = 0x30334152,
	WL_DRM_FORMAT_BGRA1010102 = 0x30334142,
	WL_DRM_FORMAT_YUYV = 0x56595559,
	WL_DRM_FORMAT_YVYU = 0x55595659,
	WL_DRM_FORMAT_UYVY = 0x59565955,
	WL_DRM_FORMAT_VYUY = 0x59555956,
	WL_DRM_FORMAT_AYUV = 0x56555941,
	WL_DRM_FORMAT_NV12 = 0x3231564e,
	WL_DRM_FORMAT_NV21 = 0x3132564e,
	WL_DRM_FORMAT_NV16 = 0x3631564e,
	WL_DRM_FORMAT_NV61 = 0x3136564e,
	WL_DRM_FORMAT_YUV410 = 0x39565559,
	WL_DRM_FORMAT_YVU410 = 0x39555659,
	WL_DRM_FORMAT_YUV411 = 0x31315559,
	WL_DRM_FORMAT_YVU411 = 0x31315659,
	WL_DRM_FORMAT_YUV420 = 0x32315559,
	WL_DRM_FORMAT_YVU420 = 0x32315659,
	WL_DRM_FORMAT_YUV422 = 0x36315559,
	WL_DRM_FORMAT_YVU422 = 0x36315659,
	WL_DRM_FORMAT_YUV444 = 0x34325559,
	WL_DRM_FORMAT_YVU444 = 0x34325659,
};
#endif /* WL_DRM_FORMAT_ENUM */

struct wl_drm;

struct wl_drm_buffer {
	struct wl_buffer buffer;
	struct wl_drm *drm;
	uint32_t format;
        const void *driver_format;
        int32_t offset[3];
        int32_t stride[3];
	void *driver_buffer;
};

struct wayland_drm_callbacks {
	int (*authenticate)(void *user_data, uint32_t id);

	void (*reference_buffer)(void *user_data, uint32_t name,
                                 struct wl_drm_buffer *buffer);

	void (*release_buffer)(void *user_data, struct wl_drm_buffer *buffer);
};

struct wl_drm *
wayland_drm_init(struct wl_display *display, char *device_name,
		 struct wayland_drm_callbacks *callbacks, void *user_data);

void
wayland_drm_uninit(struct wl_drm *drm);

int
wayland_buffer_is_drm(struct wl_buffer *buffer);

uint32_t
wayland_drm_buffer_get_format(struct wl_buffer *buffer_base);

void *
wayland_drm_buffer_get_buffer(struct wl_buffer *buffer);

#endif
