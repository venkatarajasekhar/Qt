// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GIN_PUBLIC_CONTEXT_HOLDER_H_
#define GIN_PUBLIC_CONTEXT_HOLDER_H_

#include <list>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "gin/gin_export.h"
#include "v8/include/v8.h"

namespace gin {

// Gin embedder that store embedder data in v8::Contexts must do so in a
// single field with the index kPerContextDataStartIndex + GinEmbedder-enum.
// The field at kDebugIdIndex is treated specially by V8 and is reserved for
// a V8 debugger implementation (not used by gin).
enum ContextEmbedderDataFields {
  kDebugIdIndex = 0,
  kPerContextDataStartIndex,
};

class PerContextData;

// ContextHolder is a generic class for holding a v8::Context.
class GIN_EXPORT ContextHolder {
 public:
  explicit ContextHolder(v8::Isolate* isolate);
  ~ContextHolder();

  v8::Isolate* isolate() const { return isolate_; }

  v8::Handle<v8::Context> context() const {
    return v8::Local<v8::Context>::New(isolate_, context_);
  }

  void SetContext(v8::Handle<v8::Context> context);

 private:
  v8::Isolate* isolate_;
  v8::UniquePersistent<v8::Context> context_;
  scoped_ptr<PerContextData> data_;

  DISALLOW_COPY_AND_ASSIGN(ContextHolder);
};

}  // namespace gin

#endif  // GIN_PUBLIC_CONTEXT_HOLDER_H_
