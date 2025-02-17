// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_EMF_WIN_H_
#define PRINTING_EMF_WIN_H_

#include <windows.h>

#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "printing/metafile.h"

namespace base {
class FilePath;
}

namespace gfx {
class Rect;
class Size;
}

namespace printing {

// http://msdn2.microsoft.com/en-us/library/ms535522.aspx
// Windows 2000/XP: When a page in a spooled file exceeds approximately 350
// MB, it can fail to print and not send an error message.
const size_t kMetafileMaxSize = 350*1024*1024;

// Simple wrapper class that manage an EMF data stream and its virtual HDC.
class PRINTING_EXPORT Emf : public Metafile {
 public:
  class Record;
  class Enumerator;
  struct EnumerationContext;

  // Generates a virtual HDC that will record every GDI commands and compile
  // it in a EMF data stream.
  Emf();
  virtual ~Emf();

  // Generates a new metafile that will record every GDI command, and will
  // be saved to |metafile_path|.
  virtual bool InitToFile(const base::FilePath& metafile_path);

  // Initializes the Emf with the data in |metafile_path|.
  virtual bool InitFromFile(const base::FilePath& metafile_path);

  // Metafile methods.
  virtual bool Init() OVERRIDE;
  virtual bool InitFromData(const void* src_buffer,
                            uint32 src_buffer_size) OVERRIDE;

  virtual SkBaseDevice* StartPageForVectorCanvas(
      const gfx::Size& page_size, const gfx::Rect& content_area,
      const float& scale_factor) OVERRIDE;
  // Inserts a custom GDICOMMENT records indicating StartPage/EndPage calls
  // (since StartPage and EndPage do not work in a metafile DC). Only valid
  // when hdc_ is non-NULL. |page_size|, |content_area|, and |scale_factor| are
  // ignored.
  virtual bool StartPage(const gfx::Size& page_size,
                         const gfx::Rect& content_area,
                         const float& scale_factor) OVERRIDE;
  virtual bool FinishPage() OVERRIDE;
  virtual bool FinishDocument() OVERRIDE;

  virtual uint32 GetDataSize() const OVERRIDE;
  virtual bool GetData(void* buffer, uint32 size) const OVERRIDE;

  // Saves the EMF data to a file as-is. It is recommended to use the .emf file
  // extension but it is not enforced. This function synchronously writes to the
  // file. For testing only.
  virtual bool SaveTo(const base::FilePath& file_path) const OVERRIDE;

  // Should be passed to Playback to keep the exact same size.
  virtual gfx::Rect GetPageBounds(unsigned int page_number) const OVERRIDE;

  virtual unsigned int GetPageCount() const OVERRIDE {
    return page_count_;
  }

  virtual HDC context() const OVERRIDE {
    return hdc_;
  }

  virtual bool Playback(HDC hdc, const RECT* rect) const OVERRIDE;
  virtual bool SafePlayback(HDC hdc) const OVERRIDE;

  virtual HENHMETAFILE emf() const OVERRIDE {
    return emf_;
  }

  // Returns true if metafile contains alpha blend.
  bool IsAlphaBlendUsed() const;

  // Returns new metafile with only bitmap created by playback of the current
  // metafile. Returns NULL if fails.
  Emf* RasterizeMetafile(int raster_area_in_pixels) const;

  // Returns new metafile where AlphaBlend replaced by bitmaps. Returns NULL
  // if fails.
  Emf* RasterizeAlphaBlend() const;

 private:
  FRIEND_TEST_ALL_PREFIXES(EmfTest, DC);
  FRIEND_TEST_ALL_PREFIXES(EmfPrintingTest, PageBreak);
  FRIEND_TEST_ALL_PREFIXES(EmfTest, FileBackedEmf);

  // Retrieves the underlying data stream. It is a helper function.
  bool GetDataAsVector(std::vector<uint8>* buffer) const;

  // Playbacks safely one EMF record.
  static int CALLBACK SafePlaybackProc(HDC hdc,
                                       HANDLETABLE* handle_table,
                                       const ENHMETARECORD* record,
                                       int objects_count,
                                       LPARAM param);

  // Compiled EMF data handle.
  HENHMETAFILE emf_;

  // Valid when generating EMF data through a virtual HDC.
  HDC hdc_;

  int page_count_;

  DISALLOW_COPY_AND_ASSIGN(Emf);
};

struct Emf::EnumerationContext {
  EnumerationContext();

  HANDLETABLE* handle_table;
  int objects_count;
  HDC hdc;
  const XFORM* base_matrix;
  int dc_on_page_start;
};

// One EMF record. It keeps pointers to the EMF buffer held by Emf::emf_.
// The entries become invalid once Emf::CloseEmf() is called.
class PRINTING_EXPORT Emf::Record {
 public:
  // Plays the record.
  bool Play(EnumerationContext* context) const;

  // Plays the record working around quirks with SetLayout,
  // SetWorldTransform and ModifyWorldTransform. See implementation for details.
  bool SafePlayback(EnumerationContext* context) const;

  // Access the underlying EMF record.
  const ENHMETARECORD* record() const { return record_; }

 protected:
  explicit Record(const ENHMETARECORD* record);

 private:
  friend class Emf;
  friend class Enumerator;
  const ENHMETARECORD* record_;
};

// Retrieves individual records out of a Emf buffer. The main use is to skip
// over records that are unsupported on a specific printer or to play back
// only a part of an EMF buffer.
class PRINTING_EXPORT Emf::Enumerator {
 public:
  // Iterator type used for iterating the records.
  typedef std::vector<Record>::const_iterator const_iterator;

  // Enumerates the records at construction time. |hdc| and |rect| are
  // both optional at the same time or must both be valid.
  // Warning: |emf| must be kept valid for the time this object is alive.
  Enumerator(const Emf& emf, HDC hdc, const RECT* rect);

  // Retrieves the first Record.
  const_iterator begin() const;

  // Retrieves the end of the array.
  const_iterator end() const;

 private:
  FRIEND_TEST_ALL_PREFIXES(EmfPrintingTest, Enumerate);

  // Processes one EMF record and saves it in the items_ array.
  static int CALLBACK EnhMetaFileProc(HDC hdc,
                                      HANDLETABLE* handle_table,
                                      const ENHMETARECORD* record,
                                      int objects_count,
                                      LPARAM param);

  // The collection of every EMF records in the currently loaded EMF buffer.
  // Initialized by Enumerate(). It keeps pointers to the EMF buffer held by
  // Emf::emf_. The entries become invalid once Emf::CloseEmf() is called.
  std::vector<Record> items_;

  EnumerationContext context_;

  DISALLOW_COPY_AND_ASSIGN(Enumerator);
};

}  // namespace printing

#endif  // PRINTING_EMF_WIN_H_
