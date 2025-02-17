// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PEPPER_PLUGIN_REGISTRY_H_
#define CONTENT_RENDERER_PEPPER_PEPPER_PLUGIN_REGISTRY_H_

#include <list>
#include <map>

#include "base/memory/ref_counted.h"
#include "content/public/common/pepper_plugin_info.h"

namespace content {
class PluginModule;

// This class holds references to all of the known pepper plugin modules.
//
// It keeps two lists. One list of preloaded in-process modules, and one list
// is a list of all live modules (some of which may be out-of-process and hence
// not preloaded).
class PepperPluginRegistry {
 public:
  ~PepperPluginRegistry();

  static PepperPluginRegistry* GetInstance();

  // Retrieves the information associated with the given plugin info. The
  // return value will be NULL if there is no such plugin.
  //
  // The returned pointer is owned by the PluginRegistry.
  const PepperPluginInfo* GetInfoForPlugin(const WebPluginInfo& info);

  // Returns an existing loaded module for the given path. It will search for
  // both preloaded in-process or currently active (non crashed) out-of-process
  // plugins matching the given name. Returns NULL if the plugin hasn't been
  // loaded.
  PluginModule* GetLiveModule(const base::FilePath& path);

  // Notifies the registry that a new non-preloaded module has been created.
  // This is normally called for out-of-process plugins. Once this is called,
  // the module is available to be returned by GetModule(). The module will
  // automatically unregister itself by calling PluginModuleDestroyed().
  void AddLiveModule(const base::FilePath& path, PluginModule* module);

  void PluginModuleDead(PluginModule* dead_module);

 private:
  PepperPluginRegistry();
  void Initialize();

  // All known pepper plugins.
  std::vector<PepperPluginInfo> plugin_list_;

  // Plugins that have been preloaded so they can be executed in-process in
  // the renderer (the sandbox prevents on-demand loading).
  typedef std::map<base::FilePath, scoped_refptr<PluginModule> >
      OwningModuleMap;
  OwningModuleMap preloaded_modules_;

  // A list of non-owning pointers to all currently-live plugin modules. This
  // includes both preloaded ones in preloaded_modules_, and out-of-process
  // modules whose lifetime is managed externally. This will contain only
  // non-crashed modules. If an out-of-process module crashes, it may
  // continue as long as there are WebKit references to it, but it will not
  // appear in this list.
  typedef std::map<base::FilePath, PluginModule*> NonOwningModuleMap;
  NonOwningModuleMap live_modules_;

  DISALLOW_COPY_AND_ASSIGN(PepperPluginRegistry);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PEPPER_PLUGIN_REGISTRY_H_
