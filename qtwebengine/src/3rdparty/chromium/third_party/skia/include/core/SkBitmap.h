/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkBitmap_DEFINED
#define SkBitmap_DEFINED

#include "SkColor.h"
#include "SkColorTable.h"
#include "SkImageInfo.h"
#include "SkPoint.h"
#include "SkRefCnt.h"

struct SkMask;
struct SkIRect;
struct SkRect;
class SkPaint;
class SkPixelRef;
class SkPixelRefFactory;
class SkRegion;
class SkString;
class GrTexture;

/** \class SkBitmap

    The SkBitmap class specifies a raster bitmap. A bitmap has an integer width
    and height, and a format (colortype), and a pointer to the actual pixels.
    Bitmaps can be drawn into a SkCanvas, but they are also used to specify the
    target of a SkCanvas' drawing operations.
    A const SkBitmap exposes getAddr(), which lets a caller write its pixels;
    the constness is considered to apply to the bitmap's configuration, not
    its contents.
*/
class SK_API SkBitmap {
public:
    class SK_API Allocator;

#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG
    enum Config {
        kNo_Config,         //!< bitmap has not been configured
        kA8_Config,         //!< 8-bits per pixel, with only alpha specified (0 is transparent, 0xFF is opaque)
        kIndex8_Config,     //!< 8-bits per pixel, using SkColorTable to specify the colors
        kRGB_565_Config,    //!< 16-bits per pixel, (see SkColorPriv.h for packing)
        kARGB_4444_Config,  //!< 16-bits per pixel, (see SkColorPriv.h for packing)
        kARGB_8888_Config,  //!< 32-bits per pixel, (see SkColorPriv.h for packing)
    };

    // do not add this to the Config enum, otherwise the compiler will let us
    // pass this as a valid parameter for Config.
    enum {
        kConfigCount = kARGB_8888_Config + 1
    };

    /** Return the config for the bitmap. */
    Config  config() const;
    
    SK_ATTR_DEPRECATED("use config()")
    Config  getConfig() const { return this->config(); }
#endif

    /**
     *  Default construct creates a bitmap with zero width and height, and no pixels.
     *  Its colortype is set to kUnknown_SkColorType.
     */
    SkBitmap();

    /**
     *  Copy the settings from the src into this bitmap. If the src has pixels
     *  allocated, they will be shared, not copied, so that the two bitmaps will
     *  reference the same memory for the pixels. If a deep copy is needed,
     *  where the new bitmap has its own separate copy of the pixels, use
     *  deepCopyTo().
     */
    SkBitmap(const SkBitmap& src);

    ~SkBitmap();

    /** Copies the src bitmap into this bitmap. Ownership of the src bitmap's pixels remains
        with the src bitmap.
    */
    SkBitmap& operator=(const SkBitmap& src);
    /** Swap the fields of the two bitmaps. This routine is guaranteed to never fail or throw.
    */
    //  This method is not exported to java.
    void swap(SkBitmap& other);

    ///////////////////////////////////////////////////////////////////////////

    const SkImageInfo& info() const { return fInfo; }

    int width() const { return fInfo.fWidth; }
    int height() const { return fInfo.fHeight; }
    SkColorType colorType() const { return fInfo.fColorType; }
    SkAlphaType alphaType() const { return fInfo.fAlphaType; }

#ifdef SK_SUPPORT_LEGACY_ASIMAGEINFO
    bool asImageInfo(SkImageInfo* info) const {
        // compatibility: return false for kUnknown
        if (kUnknown_SkColorType == this->colorType()) {
            return false;
        }
        if (info) {
            *info = this->info();
        }
        return true;
    }
#endif

    /**
     *  Return the number of bytes per pixel based on the colortype. If the colortype is
     *  kUnknown_SkColorType, then 0 is returned.
     */
    int bytesPerPixel() const { return fInfo.bytesPerPixel(); }

    /**
     *  Return the rowbytes expressed as a number of pixels (like width and height).
     *  If the colortype is kUnknown_SkColorType, then 0 is returned.
     */
    int rowBytesAsPixels() const {
        return fRowBytes >> this->shiftPerPixel();
    }

    /**
     *  Return the shift amount per pixel (i.e. 0 for 1-byte per pixel, 1 for 2-bytes per pixel
     *  colortypes, 2 for 4-bytes per pixel colortypes). Return 0 for kUnknown_SkColorType.
     */
    int shiftPerPixel() const { return this->bytesPerPixel() >> 1; }

    ///////////////////////////////////////////////////////////////////////////

    /** Return true iff the bitmap has empty dimensions.
     *  Hey!  Before you use this, see if you really want to know drawsNothing() instead.
     */
    bool empty() const { return fInfo.isEmpty(); }

    /** Return true iff the bitmap has no pixelref. Note: this can return true even if the
     *  dimensions of the bitmap are > 0 (see empty()).
     *  Hey!  Before you use this, see if you really want to know drawsNothing() instead.
     */
    bool isNull() const { return NULL == fPixelRef; }

    /** Return true iff drawing this bitmap has no effect.
     */
    bool drawsNothing() const { return this->empty() || this->isNull(); }

    /** Return the number of bytes between subsequent rows of the bitmap. */
    size_t rowBytes() const { return fRowBytes; }

    /**
     *  Set the bitmap's alphaType, returning true on success. If false is
     *  returned, then the specified new alphaType is incompatible with the
     *  colortype, and the current alphaType is unchanged.
     *
     *  Note: this changes the alphatype for the underlying pixels, which means
     *  that all bitmaps that might be sharing (subsets of) the pixels will
     *  be affected.
     */
    bool setAlphaType(SkAlphaType);

    /** Return the address of the pixels for this SkBitmap.
    */
    void* getPixels() const { return fPixels; }

    /** Return the byte size of the pixels, based on the height and rowBytes.
        Note this truncates the result to 32bits. Call getSize64() to detect
        if the real size exceeds 32bits.
    */
    size_t getSize() const { return fInfo.fHeight * fRowBytes; }

    /** Return the number of bytes from the pointer returned by getPixels()
        to the end of the allocated space in the buffer. Required in
        cases where extractSubset has been called.
    */
    size_t getSafeSize() const { return fInfo.getSafeSize(fRowBytes); }

    /**
     *  Return the full size of the bitmap, in bytes.
     */
    int64_t computeSize64() const {
        return sk_64_mul(fInfo.fHeight, fRowBytes);
    }

    /**
     *  Return the number of bytes from the pointer returned by getPixels()
     *  to the end of the allocated space in the buffer. This may be smaller
     *  than computeSize64() if there is any rowbytes padding beyond the width.
     */
    int64_t computeSafeSize64() const {
        return fInfo.getSafeSize64(fRowBytes);
    }

    /** Returns true if this bitmap is marked as immutable, meaning that the
        contents of its pixels will not change for the lifetime of the bitmap.
    */
    bool isImmutable() const;

    /** Marks this bitmap as immutable, meaning that the contents of its
        pixels will not change for the lifetime of the bitmap and of the
        underlying pixelref. This state can be set, but it cannot be
        cleared once it is set. This state propagates to all other bitmaps
        that share the same pixelref.
    */
    void setImmutable();

    /** Returns true if the bitmap is opaque (has no translucent/transparent pixels).
    */
    bool isOpaque() const {
        return SkAlphaTypeIsOpaque(this->alphaType());
    }

    /** Returns true if the bitmap is volatile (i.e. should not be cached by devices.)
    */
    bool isVolatile() const;

    /** Specify whether this bitmap is volatile. Bitmaps are not volatile by
        default. Temporary bitmaps that are discarded after use should be
        marked as volatile. This provides a hint to the device that the bitmap
        should not be cached. Providing this hint when appropriate can
        improve performance by avoiding unnecessary overhead and resource
        consumption on the device.
    */
    void setIsVolatile(bool);

    /** Reset the bitmap to its initial state (see default constructor). If we are a (shared)
        owner of the pixels, that ownership is decremented.
    */
    void reset();

#ifdef SK_SUPPORT_LEGACY_COMPUTE_CONFIG_SIZE
    /** Given a config and a width, this computes the optimal rowBytes value. This is called automatically
        if you pass 0 for rowBytes to setConfig().
    */
    static size_t ComputeRowBytes(Config c, int width);

    /** Return the bytes-per-pixel for the specified config. If the config is
        not at least 1-byte per pixel, return 0, including for kNo_Config.
    */
    static int ComputeBytesPerPixel(Config c);

    /** Return the shift-per-pixel for the specified config. If the config is
     not at least 1-byte per pixel, return 0, including for kNo_Config.
     */
    static int ComputeShiftPerPixel(Config c) {
        return ComputeBytesPerPixel(c) >> 1;
    }

    static int64_t ComputeSize64(Config, int width, int height);
    static size_t ComputeSize(Config, int width, int height);
#endif

    /**
     *  This will brute-force return true if all of the pixels in the bitmap
     *  are opaque. If it fails to read the pixels, or encounters an error,
     *  it will return false.
     *
     *  Since this can be an expensive operation, the bitmap stores a flag for
     *  this (isOpaque). Only call this if you need to compute this value from
     *  "unknown" pixels.
     */
    static bool ComputeIsOpaque(const SkBitmap&);

    /**
     *  Return the bitmap's bounds [0, 0, width, height] as an SkRect
     */
    void getBounds(SkRect* bounds) const;
    void getBounds(SkIRect* bounds) const;

#ifdef SK_SUPPORT_LEGACY_SETCONFIG
    /** Set the bitmap's config and dimensions. If rowBytes is 0, then
        ComputeRowBytes() is called to compute the optimal value. This resets
        any pixel/colortable ownership, just like reset().
    */
    bool setConfig(Config, int width, int height, size_t rowBytes, SkAlphaType);

    bool setConfig(Config config, int width, int height, size_t rowBytes = 0) {
        return this->setConfig(config, width, height, rowBytes,
                               kPremul_SkAlphaType);
    }
#endif

    bool setInfo(const SkImageInfo&, size_t rowBytes = 0);

#ifdef SK_SUPPORT_LEGACY_SETCONFIG_INFO
    bool setConfig(const SkImageInfo& info, size_t rowBytes = 0) {
        return this->setInfo(info, rowBytes);
    }
#endif

    /**
     *  Allocate a pixelref to match the specified image info. If the Factory
     *  is non-null, call it to allcoate the pixelref. If the ImageInfo requires
     *  a colortable, then ColorTable must be non-null, and will be ref'd.
     *  On failure, the bitmap will be set to empty and return false.
     */
    bool allocPixels(const SkImageInfo&, SkPixelRefFactory*, SkColorTable*);

    /**
     *  Allocate a pixelref to match the specified image info, using the default
     *  allocator.
     *  On success, the bitmap's pixels will be "locked", and return true.
     *  On failure, the bitmap will be set to empty and return false.
     */
    bool allocPixels(const SkImageInfo& info) {
        return this->allocPixels(info, NULL, NULL);
    }

    bool allocN32Pixels(int width, int height, bool isOpaque = false) {
        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        if (isOpaque) {
            info.fAlphaType = kOpaque_SkAlphaType;
        }
        return this->allocPixels(info);
    }

    /**
     *  Install a pixelref that wraps the specified pixels and rowBytes, and
     *  optional ReleaseProc and context. When the pixels are no longer
     *  referenced, if releaseProc is not null, it will be called with the
     *  pixels and context as parameters.
     *  On failure, the bitmap will be set to empty and return false.
     */
    bool installPixels(const SkImageInfo&, void* pixels, size_t rowBytes, SkColorTable*,
                       void (*releaseProc)(void* addr, void* context), void* context);

#ifdef SK_SUPPORT_LEGACY_INSTALLPIXELSPARAMS
    bool installPixels(const SkImageInfo& info, void* pixels, size_t rowBytes,
                       void (*releaseProc)(void* addr, void* context),
                       void* context) {
        return this->installPixels(info, pixels, rowBytes, NULL, releaseProc, context);
    }
#endif

    /**
     *  Call installPixels with no ReleaseProc specified. This means that the
     *  caller must ensure that the specified pixels are valid for the lifetime
     *  of the created bitmap (and its pixelRef).
     */
    bool installPixels(const SkImageInfo& info, void* pixels, size_t rowBytes) {
        return this->installPixels(info, pixels, rowBytes, NULL, NULL, NULL);
    }

    /**
     *  Calls installPixels() with the value in the SkMask. The caller must
     *  ensure that the specified mask pixels are valid for the lifetime
     *  of the created bitmap (and its pixelRef).
     */
    bool installMaskPixels(const SkMask&);

    /** Use this to assign a new pixel address for an existing bitmap. This
        will automatically release any pixelref previously installed. Only call
        this if you are handling ownership/lifetime of the pixel memory.

        If the bitmap retains a reference to the colortable (assuming it is
        not null) it will take care of incrementing the reference count.

        @param pixels   Address for the pixels, managed by the caller.
        @param ctable   ColorTable (or null) that matches the specified pixels
    */
    void setPixels(void* p, SkColorTable* ctable = NULL);

    /** Copies the bitmap's pixels to the location pointed at by dst and returns
        true if possible, returns false otherwise.

        In the case when the dstRowBytes matches the bitmap's rowBytes, the copy
        may be made faster by copying over the dst's per-row padding (for all
        rows but the last). By setting preserveDstPad to true the caller can
        disable this optimization and ensure that pixels in the padding are not
        overwritten.

        Always returns false for RLE formats.

        @param dst      Location of destination buffer.
        @param dstSize  Size of destination buffer. Must be large enough to hold
                        pixels using indicated stride.
        @param dstRowBytes  Width of each line in the buffer. If 0, uses
                            bitmap's internal stride.
        @param preserveDstPad Must we preserve padding in the dst
    */
    bool copyPixelsTo(void* const dst, size_t dstSize, size_t dstRowBytes = 0,
                      bool preserveDstPad = false) const;

    /** Use the standard HeapAllocator to create the pixelref that manages the
        pixel memory. It will be sized based on the current ImageInfo.
        If this is called multiple times, a new pixelref object will be created
        each time.

        If the bitmap retains a reference to the colortable (assuming it is
        not null) it will take care of incrementing the reference count.

        @param ctable   ColorTable (or null) to use with the pixels that will
                        be allocated. Only used if colortype == kIndex_8_SkColorType
        @return true if the allocation succeeds. If not the pixelref field of
                     the bitmap will be unchanged.
    */
    bool allocPixels(SkColorTable* ctable = NULL) {
        return this->allocPixels(NULL, ctable);
    }

    /** Use the specified Allocator to create the pixelref that manages the
        pixel memory. It will be sized based on the current ImageInfo.
        If this is called multiple times, a new pixelref object will be created
        each time.

        If the bitmap retains a reference to the colortable (assuming it is
        not null) it will take care of incrementing the reference count.

        @param allocator The Allocator to use to create a pixelref that can
                         manage the pixel memory for the current ImageInfo.
                         If allocator is NULL, the standard HeapAllocator will be used.
        @param ctable   ColorTable (or null) to use with the pixels that will
                        be allocated. Only used if colortype == kIndex_8_SkColorType.
                        If it is non-null and the colortype is not indexed, it will
                        be ignored.
        @return true if the allocation succeeds. If not the pixelref field of
                     the bitmap will be unchanged.
    */
    bool allocPixels(Allocator* allocator, SkColorTable* ctable);

    /**
     *  Return the current pixelref object or NULL if there is none. This does
     *  not affect the refcount of the pixelref.
     */
    SkPixelRef* pixelRef() const { return fPixelRef; }

    /**
     *  A bitmap can reference a subset of a pixelref's pixels. That means the
     *  bitmap's width/height can be <= the dimensions of the pixelref. The
     *  pixelref origin is the x,y location within the pixelref's pixels for
     *  the bitmap's top/left corner. To be valid the following must be true:
     *
     *  origin_x + bitmap_width  <= pixelref_width
     *  origin_y + bitmap_height <= pixelref_height
     *
     *  pixelRefOrigin() returns this origin, or (0,0) if there is no pixelRef.
     */
    SkIPoint pixelRefOrigin() const { return fPixelRefOrigin; }

    /**
     *  Assign a pixelref and origin to the bitmap. Pixelrefs are reference,
     *  so the existing one (if any) will be unref'd and the new one will be
     *  ref'd. (x,y) specify the offset within the pixelref's pixels for the
     *  top/left corner of the bitmap. For a bitmap that encompases the entire
     *  pixels of the pixelref, these will be (0,0).
     */
    SkPixelRef* setPixelRef(SkPixelRef* pr, int dx, int dy);

    SkPixelRef* setPixelRef(SkPixelRef* pr, const SkIPoint& origin) {
        return this->setPixelRef(pr, origin.fX, origin.fY);
    }

    SkPixelRef* setPixelRef(SkPixelRef* pr) {
        return this->setPixelRef(pr, 0, 0);
    }

    /** Call this to ensure that the bitmap points to the current pixel address
        in the pixelref. Balance it with a call to unlockPixels(). These calls
        are harmless if there is no pixelref.
    */
    void lockPixels() const;
    /** When you are finished access the pixel memory, call this to balance a
        previous call to lockPixels(). This allows pixelrefs that implement
        cached/deferred image decoding to know when there are active clients of
        a given image.
    */
    void unlockPixels() const;

    /**
     *  Some bitmaps can return a copy of their pixels for lockPixels(), but
     *  that copy, if modified, will not be pushed back. These bitmaps should
     *  not be used as targets for a raster device/canvas (since all pixels
     *  modifications will be lost when unlockPixels() is called.)
     */
    bool lockPixelsAreWritable() const;

    /** Call this to be sure that the bitmap is valid enough to be drawn (i.e.
        it has non-null pixels, and if required by its colortype, it has a
        non-null colortable. Returns true if all of the above are met.
    */
    bool readyToDraw() const {
        return this->getPixels() != NULL &&
               (this->colorType() != kIndex_8_SkColorType || NULL != fColorTable);
    }

    /** Returns the pixelRef's texture, or NULL
     */
    GrTexture* getTexture() const;

    /** Return the bitmap's colortable, if it uses one (i.e. colorType is
        Index_8) and the pixels are locked.
        Otherwise returns NULL. Does not affect the colortable's
        reference count.
    */
    SkColorTable* getColorTable() const { return fColorTable; }

    /** Returns a non-zero, unique value corresponding to the pixels in our
        pixelref. Each time the pixels are changed (and notifyPixelsChanged
        is called), a different generation ID will be returned. Finally, if
        there is no pixelRef then zero is returned.
    */
    uint32_t getGenerationID() const;

    /** Call this if you have changed the contents of the pixels. This will in-
        turn cause a different generation ID value to be returned from
        getGenerationID().
    */
    void notifyPixelsChanged() const;

    /**
     *  Fill the entire bitmap with the specified color.
     *  If the bitmap's colortype does not support alpha (e.g. 565) then the alpha
     *  of the color is ignored (treated as opaque). If the colortype only supports
     *  alpha (e.g. A1 or A8) then the color's r,g,b components are ignored.
     */
    void eraseColor(SkColor c) const {
        this->eraseARGB(SkColorGetA(c), SkColorGetR(c), SkColorGetG(c),
                        SkColorGetB(c));
    }

    /**
     *  Fill the entire bitmap with the specified color.
     *  If the bitmap's colortype does not support alpha (e.g. 565) then the alpha
     *  of the color is ignored (treated as opaque). If the colortype only supports
     *  alpha (e.g. A1 or A8) then the color's r,g,b components are ignored.
     */
    void eraseARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b) const;

    SK_ATTR_DEPRECATED("use eraseARGB or eraseColor")
    void eraseRGB(U8CPU r, U8CPU g, U8CPU b) const {
        this->eraseARGB(0xFF, r, g, b);
    }

    /**
     *  Fill the specified area of this bitmap with the specified color.
     *  If the bitmap's colortype does not support alpha (e.g. 565) then the alpha
     *  of the color is ignored (treated as opaque). If the colortype only supports
     *  alpha (e.g. A1 or A8) then the color's r,g,b components are ignored.
     */
    void eraseArea(const SkIRect& area, SkColor c) const;

    /** Scroll (a subset of) the contents of this bitmap by dx/dy. If there are
        no pixels allocated (i.e. getPixels() returns null) the method will
        still update the inval region (if present). If the bitmap is immutable,
        do nothing and return false.

        @param subset The subset of the bitmap to scroll/move. To scroll the
                      entire contents, specify [0, 0, width, height] or just
                      pass null.
        @param dx The amount to scroll in X
        @param dy The amount to scroll in Y
        @param inval Optional (may be null). Returns the area of the bitmap that
                     was scrolled away. E.g. if dx = dy = 0, then inval would
                     be set to empty. If dx >= width or dy >= height, then
                     inval would be set to the entire bounds of the bitmap.
        @return true if the scroll was doable. Will return false if the colortype is kUnkown or
                     if the bitmap is immutable.
                     If no pixels are present (i.e. getPixels() returns false)
                     inval will still be updated, and true will be returned.
    */
    bool scrollRect(const SkIRect* subset, int dx, int dy,
                    SkRegion* inval = NULL) const;

    /**
     *  Return the SkColor of the specified pixel.  In most cases this will
     *  require un-premultiplying the color.  Alpha only colortypes (e.g. kAlpha_8_SkColorType)
     *  return black with the appropriate alpha set.  The value is undefined
     *  for kUnknown_SkColorType or if x or y are out of bounds, or if the bitmap
     *  does not have any pixels (or has not be locked with lockPixels()).
     */
    SkColor getColor(int x, int y) const;

    /** Returns the address of the specified pixel. This performs a runtime
        check to know the size of the pixels, and will return the same answer
        as the corresponding size-specific method (e.g. getAddr16). Since the
        check happens at runtime, it is much slower than using a size-specific
        version. Unlike the size-specific methods, this routine also checks if
        getPixels() returns null, and returns that. The size-specific routines
        perform a debugging assert that getPixels() is not null, but they do
        not do any runtime checks.
    */
    void* getAddr(int x, int y) const;

    /** Returns the address of the pixel specified by x,y for 32bit pixels.
     *  In debug build, this asserts that the pixels are allocated and locked,
     *  and that the colortype is 32-bit, however none of these checks are performed
     *  in the release build.
     */
    inline uint32_t* getAddr32(int x, int y) const;

    /** Returns the address of the pixel specified by x,y for 16bit pixels.
     *  In debug build, this asserts that the pixels are allocated and locked,
     *  and that the colortype is 16-bit, however none of these checks are performed
     *  in the release build.
     */
    inline uint16_t* getAddr16(int x, int y) const;

    /** Returns the address of the pixel specified by x,y for 8bit pixels.
     *  In debug build, this asserts that the pixels are allocated and locked,
     *  and that the colortype is 8-bit, however none of these checks are performed
     *  in the release build.
     */
    inline uint8_t* getAddr8(int x, int y) const;

    /** Returns the color corresponding to the pixel specified by x,y for
     *  colortable based bitmaps.
     *  In debug build, this asserts that the pixels are allocated and locked,
     *  that the colortype is indexed, and that the colortable is allocated,
     *  however none of these checks are performed in the release build.
     */
    inline SkPMColor getIndex8Color(int x, int y) const;

    /** Set dst to be a setset of this bitmap. If possible, it will share the
        pixel memory, and just point into a subset of it. However, if the colortype
        does not support this, a local copy will be made and associated with
        the dst bitmap. If the subset rectangle, intersected with the bitmap's
        dimensions is empty, or if there is an unsupported colortype, false will be
        returned and dst will be untouched.
        @param dst  The bitmap that will be set to a subset of this bitmap
        @param subset The rectangle of pixels in this bitmap that dst will
                      reference.
        @return true if the subset copy was successfully made.
    */
    bool extractSubset(SkBitmap* dst, const SkIRect& subset) const;

    /** Makes a deep copy of this bitmap, respecting the requested colorType,
     *  and allocating the dst pixels on the cpu.
     *  Returns false if either there is an error (i.e. the src does not have
     *  pixels) or the request cannot be satisfied (e.g. the src has per-pixel
     *  alpha, and the requested colortype does not support alpha).
     *  @param dst The bitmap to be sized and allocated
     *  @param ct The desired colorType for dst
     *  @param allocator Allocator used to allocate the pixelref for the dst
     *                   bitmap. If this is null, the standard HeapAllocator
     *                   will be used.
     *  @return true if the copy was made.
     */
    bool copyTo(SkBitmap* dst, SkColorType ct, Allocator* = NULL) const;

    bool copyTo(SkBitmap* dst, Allocator* allocator = NULL) const {
        return this->copyTo(dst, this->colorType(), allocator);
    }

    /**
     *  Returns true if this bitmap's pixels can be converted into the requested
     *  colorType, such that copyTo() could succeed.
     */
    bool canCopyTo(SkColorType colorType) const;

    /** Makes a deep copy of this bitmap, keeping the copied pixels
     *  in the same domain as the source: If the src pixels are allocated for
     *  the cpu, then so will the dst. If the src pixels are allocated on the
     *  gpu (typically as a texture), the it will do the same for the dst.
     *  If the request cannot be fulfilled, returns false and dst is unmodified.
     */
    bool deepCopyTo(SkBitmap* dst) const;

#ifdef SK_BUILD_FOR_ANDROID
    bool hasHardwareMipMap() const {
        return (fFlags & kHasHardwareMipMap_Flag) != 0;
    }

    void setHasHardwareMipMap(bool hasHardwareMipMap) {
        if (hasHardwareMipMap) {
            fFlags |= kHasHardwareMipMap_Flag;
        } else {
            fFlags &= ~kHasHardwareMipMap_Flag;
        }
    }
#endif

    bool extractAlpha(SkBitmap* dst) const {
        return this->extractAlpha(dst, NULL, NULL, NULL);
    }

    bool extractAlpha(SkBitmap* dst, const SkPaint* paint,
                      SkIPoint* offset) const {
        return this->extractAlpha(dst, paint, NULL, offset);
    }

    /** Set dst to contain alpha layer of this bitmap. If destination bitmap
        fails to be initialized, e.g. because allocator can't allocate pixels
        for it, dst will not be modified and false will be returned.

        @param dst The bitmap to be filled with alpha layer
        @param paint The paint to draw with
        @param allocator Allocator used to allocate the pixelref for the dst
                         bitmap. If this is null, the standard HeapAllocator
                         will be used.
        @param offset If not null, it is set to top-left coordinate to position
                      the returned bitmap so that it visually lines up with the
                      original
    */
    bool extractAlpha(SkBitmap* dst, const SkPaint* paint, Allocator* allocator,
                      SkIPoint* offset) const;

    SkDEBUGCODE(void validate() const;)

    class Allocator : public SkRefCnt {
    public:
        SK_DECLARE_INST_COUNT(Allocator)

        /** Allocate the pixel memory for the bitmap, given its dimensions and
            colortype. Return true on success, where success means either setPixels
            or setPixelRef was called. The pixels need not be locked when this
            returns. If the colortype requires a colortable, it also must be
            installed via setColorTable. If false is returned, the bitmap and
            colortable should be left unchanged.
        */
        virtual bool allocPixelRef(SkBitmap*, SkColorTable*) = 0;
    private:
        typedef SkRefCnt INHERITED;
    };

    /** Subclass of Allocator that returns a pixelref that allocates its pixel
        memory from the heap. This is the default Allocator invoked by
        allocPixels().
    */
    class HeapAllocator : public Allocator {
    public:
        virtual bool allocPixelRef(SkBitmap*, SkColorTable*) SK_OVERRIDE;
    };

    class RLEPixels {
    public:
        RLEPixels(int width, int height);
        virtual ~RLEPixels();

        uint8_t* packedAtY(int y) const {
            SkASSERT((unsigned)y < (unsigned)fHeight);
            return fYPtrs[y];
        }

        // called by subclasses during creation
        void setPackedAtY(int y, uint8_t* addr) {
            SkASSERT((unsigned)y < (unsigned)fHeight);
            fYPtrs[y] = addr;
        }

    private:
        uint8_t** fYPtrs;
        int       fHeight;
    };

    SK_TO_STRING_NONVIRT()

private:
    mutable SkPixelRef* fPixelRef;
    mutable int         fPixelLockCount;
    // These are just caches from the locked pixelref
    mutable void*       fPixels;
    mutable SkColorTable* fColorTable;    // only meaningful for kIndex8

    SkIPoint    fPixelRefOrigin;

    enum Flags {
        kImageIsOpaque_Flag     = 0x01,
        kImageIsVolatile_Flag   = 0x02,
        kImageIsImmutable_Flag  = 0x04,
#ifdef SK_BUILD_FOR_ANDROID
        /* A hint for the renderer responsible for drawing this bitmap
         * indicating that it should attempt to use mipmaps when this bitmap
         * is drawn scaled down.
         */
        kHasHardwareMipMap_Flag = 0x08,
#endif
    };

    SkImageInfo fInfo;

    uint32_t    fRowBytes;

    uint8_t     fFlags;

    void internalErase(const SkIRect&, U8CPU a, U8CPU r, U8CPU g, U8CPU b)const;

    /*  Unreference any pixelrefs or colortables
    */
    void freePixels();
    void updatePixelsFromRef() const;

    void legacyUnflatten(SkReadBuffer&);

    static void WriteRawPixels(SkWriteBuffer*, const SkBitmap&);
    static bool ReadRawPixels(SkReadBuffer*, SkBitmap*);

    friend class SkBitmapSource;    // unflatten
    friend class SkReadBuffer;      // unflatten, rawpixels
    friend class SkWriteBuffer;     // rawpixels
    friend struct SkBitmapProcState;
};

class SkAutoLockPixels : SkNoncopyable {
public:
    SkAutoLockPixels(const SkBitmap& bm, bool doLock = true) : fBitmap(bm) {
        fDidLock = doLock;
        if (doLock) {
            bm.lockPixels();
        }
    }
    ~SkAutoLockPixels() {
        if (fDidLock) {
            fBitmap.unlockPixels();
        }
    }

private:
    const SkBitmap& fBitmap;
    bool            fDidLock;
};
//TODO(mtklein): uncomment when 71713004 lands and Chromium's fixed.
//#define SkAutoLockPixels(...) SK_REQUIRE_LOCAL_VAR(SkAutoLockPixels)

/** Helper class that performs the lock/unlockColors calls on a colortable.
    The destructor will call unlockColors(false) if it has a bitmap's colortable
*/
class SkAutoLockColors : SkNoncopyable {
public:
    /** Initialize with no bitmap. Call lockColors(bitmap) to lock bitmap's
        colortable
     */
    SkAutoLockColors() : fCTable(NULL), fColors(NULL) {}
    /** Initialize with bitmap, locking its colortable if present
     */
    explicit SkAutoLockColors(const SkBitmap& bm) {
        fCTable = bm.getColorTable();
        fColors = fCTable ? fCTable->lockColors() : NULL;
    }
    /** Initialize with a colortable (may be null)
     */
    explicit SkAutoLockColors(SkColorTable* ctable) {
        fCTable = ctable;
        fColors = ctable ? ctable->lockColors() : NULL;
    }
    ~SkAutoLockColors() {
        if (fCTable) {
            fCTable->unlockColors();
        }
    }

    /** Return the currently locked colors, or NULL if no bitmap's colortable
        is currently locked.
    */
    const SkPMColor* colors() const { return fColors; }

    /** Locks the table and returns is colors (assuming ctable is not null) and
        unlocks the previous table if one was present
     */
    const SkPMColor* lockColors(SkColorTable* ctable) {
        if (fCTable) {
            fCTable->unlockColors();
        }
        fCTable = ctable;
        fColors = ctable ? ctable->lockColors() : NULL;
        return fColors;
    }

    const SkPMColor* lockColors(const SkBitmap& bm) {
        return this->lockColors(bm.getColorTable());
    }

private:
    SkColorTable*    fCTable;
    const SkPMColor* fColors;
};
#define SkAutoLockColors(...) SK_REQUIRE_LOCAL_VAR(SkAutoLockColors)

///////////////////////////////////////////////////////////////////////////////

inline uint32_t* SkBitmap::getAddr32(int x, int y) const {
    SkASSERT(fPixels);
    SkASSERT(4 == this->bytesPerPixel());
    SkASSERT((unsigned)x < (unsigned)this->width() && (unsigned)y < (unsigned)this->height());
    return (uint32_t*)((char*)fPixels + y * fRowBytes + (x << 2));
}

inline uint16_t* SkBitmap::getAddr16(int x, int y) const {
    SkASSERT(fPixels);
    SkASSERT(2 == this->bytesPerPixel());
    SkASSERT((unsigned)x < (unsigned)this->width() && (unsigned)y < (unsigned)this->height());
    return (uint16_t*)((char*)fPixels + y * fRowBytes + (x << 1));
}

inline uint8_t* SkBitmap::getAddr8(int x, int y) const {
    SkASSERT(fPixels);
    SkASSERT(1 == this->bytesPerPixel());
    SkASSERT((unsigned)x < (unsigned)this->width() && (unsigned)y < (unsigned)this->height());
    return (uint8_t*)fPixels + y * fRowBytes + x;
}

inline SkPMColor SkBitmap::getIndex8Color(int x, int y) const {
    SkASSERT(fPixels);
    SkASSERT(kIndex_8_SkColorType == this->colorType());
    SkASSERT((unsigned)x < (unsigned)this->width() && (unsigned)y < (unsigned)this->height());
    SkASSERT(fColorTable);
    return (*fColorTable)[*((const uint8_t*)fPixels + y * fRowBytes + x)];
}

#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG
///////////////////////////////////////////////////////////////////////////////
//
// Helpers until we can fully deprecate SkBitmap::Config
//
extern SkBitmap::Config SkColorTypeToBitmapConfig(SkColorType);
extern SkColorType SkBitmapConfigToColorType(SkBitmap::Config);
#endif

#endif
