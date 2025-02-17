/*
 * libjingle
 * Copyright 2004--2010, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WEBRTC_AUDIO_DEVICE_LATEBINDINGSYMBOLTABLE_LINUX_H
#define WEBRTC_AUDIO_DEVICE_LATEBINDINGSYMBOLTABLE_LINUX_H

#include <assert.h>
#include <stddef.h>  // for NULL
#include <string.h>

#include "webrtc/base/constructormagic.h"
#include "webrtc/system_wrappers/interface/trace.h"

// This file provides macros for creating "symbol table" classes to simplify the
// dynamic loading of symbols from DLLs. Currently the implementation only
// supports Linux and pure C symbols.
// See talk/sound/pulseaudiosymboltable.(h|cc) for an example.

namespace webrtc_adm_linux {

#ifdef WEBRTC_LINUX
typedef void *DllHandle;

const DllHandle kInvalidDllHandle = NULL;
#else
#error Not implemented
#endif

// These are helpers for use only by the class below.
DllHandle InternalLoadDll(const char dll_name[]);

void InternalUnloadDll(DllHandle handle);

bool InternalLoadSymbols(DllHandle handle,
                         int num_symbols,
                         const char *const symbol_names[],
                         void *symbols[]);

template <int SYMBOL_TABLE_SIZE,
          const char kDllName[],
          const char *const kSymbolNames[]>
class LateBindingSymbolTable {
 public:
  LateBindingSymbolTable()
      : handle_(kInvalidDllHandle),
        undefined_symbols_(false) {
    memset(symbols_, 0, sizeof(symbols_));
  }

  ~LateBindingSymbolTable() {
    Unload();
  }

  static int NumSymbols() {
    return SYMBOL_TABLE_SIZE;
  }

  // We do not use this, but we offer it for theoretical convenience.
  static const char *GetSymbolName(int index) {
    assert(index < NumSymbols());
    return kSymbolNames[index];
  }

  bool IsLoaded() const {
    return handle_ != kInvalidDllHandle;
  }

  // Loads the DLL and the symbol table. Returns true iff the DLL and symbol
  // table loaded successfully.
  bool Load() {
    if (IsLoaded()) {
      return true;
    }
    if (undefined_symbols_) {
      // We do not attempt to load again because repeated attempts are not
      // likely to succeed and DLL loading is costly.
      //WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
      //           "We know there are undefined symbols");
      return false;
    }
    handle_ = InternalLoadDll(kDllName);
    if (!IsLoaded()) {
      return false;
    }
    if (!InternalLoadSymbols(handle_, NumSymbols(), kSymbolNames, symbols_)) {
      undefined_symbols_ = true;
      Unload();
      return false;
    }
    return true;
  }

  void Unload() {
    if (!IsLoaded()) {
      return;
    }
    InternalUnloadDll(handle_);
    handle_ = kInvalidDllHandle;
    memset(symbols_, 0, sizeof(symbols_));
  }

  // Retrieves the given symbol. NOTE: Recommended to use LATESYM_GET below
  // instead of this.
  void *GetSymbol(int index) const {
    assert(IsLoaded());
    assert(index < NumSymbols());
    return symbols_[index];
  }

 private:
  DllHandle handle_;
  bool undefined_symbols_;
  void *symbols_[SYMBOL_TABLE_SIZE];

  DISALLOW_COPY_AND_ASSIGN(LateBindingSymbolTable);
};

// This macro must be invoked in a header to declare a symbol table class.
#define LATE_BINDING_SYMBOL_TABLE_DECLARE_BEGIN(ClassName) \
enum {

// This macro must be invoked in the header declaration once for each symbol
// (recommended to use an X-Macro to avoid duplication).
// This macro defines an enum with names built from the symbols, which
// essentially creates a hash table in the compiler from symbol names to their
// indices in the symbol table class.
#define LATE_BINDING_SYMBOL_TABLE_DECLARE_ENTRY(ClassName, sym) \
  ClassName##_SYMBOL_TABLE_INDEX_##sym,

// This macro completes the header declaration.
#define LATE_BINDING_SYMBOL_TABLE_DECLARE_END(ClassName) \
  ClassName##_SYMBOL_TABLE_SIZE \
}; \
\
extern const char ClassName##_kDllName[]; \
extern const char *const \
    ClassName##_kSymbolNames[ClassName##_SYMBOL_TABLE_SIZE]; \
\
typedef ::webrtc_adm_linux::LateBindingSymbolTable<ClassName##_SYMBOL_TABLE_SIZE, \
                                            ClassName##_kDllName, \
                                            ClassName##_kSymbolNames> \
    ClassName;

// This macro must be invoked in a .cc file to define a previously-declared
// symbol table class.
#define LATE_BINDING_SYMBOL_TABLE_DEFINE_BEGIN(ClassName, dllName) \
const char ClassName##_kDllName[] = dllName; \
const char *const ClassName##_kSymbolNames[ClassName##_SYMBOL_TABLE_SIZE] = {

// This macro must be invoked in the .cc definition once for each symbol
// (recommended to use an X-Macro to avoid duplication).
// This would have to use the mangled name if we were to ever support C++
// symbols.
#define LATE_BINDING_SYMBOL_TABLE_DEFINE_ENTRY(ClassName, sym) \
  #sym,

#define LATE_BINDING_SYMBOL_TABLE_DEFINE_END(ClassName) \
};

// Index of a given symbol in the given symbol table class.
#define LATESYM_INDEXOF(ClassName, sym) \
  (ClassName##_SYMBOL_TABLE_INDEX_##sym)

// Returns a reference to the given late-binded symbol, with the correct type.
#define LATESYM_GET(ClassName, inst, sym) \
  (*reinterpret_cast<typeof(&sym)>( \
      (inst)->GetSymbol(LATESYM_INDEXOF(ClassName, sym))))

}  // namespace webrtc_adm_linux

#endif  // WEBRTC_ADM_LATEBINDINGSYMBOLTABLE_LINUX_H
