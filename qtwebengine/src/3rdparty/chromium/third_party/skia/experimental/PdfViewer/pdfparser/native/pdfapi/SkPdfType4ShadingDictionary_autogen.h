/*
 * Copyright 2013 Google Inc.

 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkPdfType4ShadingDictionary_DEFINED
#define SkPdfType4ShadingDictionary_DEFINED

#include "SkPdfShadingDictionary_autogen.h"

// Additional entries specific to a type 4 shading dictionary
class SkPdfType4ShadingDictionary : public SkPdfShadingDictionary {
public:
public:
   SkPdfType4ShadingDictionary* asType4ShadingDictionary() {return this;}
   const SkPdfType4ShadingDictionary* asType4ShadingDictionary() const {return this;}

private:
   SkPdfType1ShadingDictionary* asType1ShadingDictionary() {return (SkPdfType1ShadingDictionary*)this;}
   const SkPdfType1ShadingDictionary* asType1ShadingDictionary() const {return (const SkPdfType1ShadingDictionary*)this;}

   SkPdfType2ShadingDictionary* asType2ShadingDictionary() {return (SkPdfType2ShadingDictionary*)this;}
   const SkPdfType2ShadingDictionary* asType2ShadingDictionary() const {return (const SkPdfType2ShadingDictionary*)this;}

   SkPdfType3ShadingDictionary* asType3ShadingDictionary() {return (SkPdfType3ShadingDictionary*)this;}
   const SkPdfType3ShadingDictionary* asType3ShadingDictionary() const {return (const SkPdfType3ShadingDictionary*)this;}

   SkPdfType5ShadingDictionary* asType5ShadingDictionary() {return (SkPdfType5ShadingDictionary*)this;}
   const SkPdfType5ShadingDictionary* asType5ShadingDictionary() const {return (const SkPdfType5ShadingDictionary*)this;}

   SkPdfType6ShadingDictionary* asType6ShadingDictionary() {return (SkPdfType6ShadingDictionary*)this;}
   const SkPdfType6ShadingDictionary* asType6ShadingDictionary() const {return (const SkPdfType6ShadingDictionary*)this;}

public:
   bool valid() const {return true;}
  int64_t BitsPerCoordinate(SkPdfNativeDoc* doc);
  bool has_BitsPerCoordinate() const;
  int64_t BitsPerComponent(SkPdfNativeDoc* doc);
  bool has_BitsPerComponent() const;
  int64_t BitsPerFlag(SkPdfNativeDoc* doc);
  bool has_BitsPerFlag() const;
  SkRect Decode(SkPdfNativeDoc* doc);
  bool has_Decode() const;
  SkPdfFunction Function(SkPdfNativeDoc* doc);
  bool has_Function() const;
};

#endif  // SkPdfType4ShadingDictionary_DEFINED
