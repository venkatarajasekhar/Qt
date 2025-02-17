// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SKIA_BENCHMARKING_EXTENSION_H_
#define CONTENT_RENDERER_SKIA_BENCHMARKING_EXTENSION_H_

#include "base/basictypes.h"
#include "gin/wrappable.h"

namespace blink {
class WebFrame;
}

namespace gin {
class Arguments;
}

namespace content {

class SkiaBenchmarking : public gin::Wrappable<SkiaBenchmarking> {
 public:
  static gin::WrapperInfo kWrapperInfo;
  static void Install(blink::WebFrame* frame);

  // Wrapper around SkGraphics::Init that can be invoked multiple times.
  static void Initialize();

 private:
  SkiaBenchmarking();
  virtual ~SkiaBenchmarking();

  // gin::Wrappable.
  virtual gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) OVERRIDE;

  // Rasterizes a Picture JSON-encoded by cc::Picture::AsValue().
  //
  // Takes a JSON-encoded cc::Picture and optionally rasterization parameters:
  //   {
  //     'scale':    {Number},
  //     'stop':     {Number},
  //     'overdraw': {Boolean},
  //     'clip':     [Number, Number, Number, Number]
  //   }
  //
  // Returns
  //  {
  //    'width':    {Number},
  //    'height':   {Number},
  //    'data':     {ArrayBuffer}
  //  }
  void Rasterize(gin::Arguments* args);

  // Extracts the Skia draw commands from a JSON-encoded cc::Picture.
  //
  // Takes a JSON-encoded cc::Picture and returns
  // [{ 'cmd': {String}, 'info': [String, ...] }, ...]
  void GetOps(gin::Arguments* args);

  // Returns timing information for the given picture.
  //
  // Takes a JSON-encoded cc::Picture and returns
  // { 'total_time': {Number}, 'cmd_times': [Number, ...] }
  void GetOpTimings(gin::Arguments* args);

  // Returns meta information for the given picture.
  //
  // Takes a base64 encoded SKP and returns
  // { 'width': {Number}, 'height': {Number} }
  void GetInfo(gin::Arguments* args);

  DISALLOW_COPY_AND_ASSIGN(SkiaBenchmarking);
};

}  // namespace content

#endif // CONTENT_RENDERER_SKIA_BENCHMARKING_EXTENSION_H_
