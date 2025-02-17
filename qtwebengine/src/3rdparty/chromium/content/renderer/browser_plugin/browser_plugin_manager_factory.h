// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BROWSER_PLUGIN_BROWSER_PLUGIN_MANAGER_FACTORY_H_
#define CONTENT_BROWSER_BROWSER_PLUGIN_BROWSER_PLUGIN_MANAGER_FACTORY_H_

namespace content {

class CONTENT_EXPORT BrowserPluginManagerFactory {
 public:
  virtual BrowserPluginManager* CreateBrowserPluginManager(
      RenderViewImpl* render_view) = 0;

 protected:
  virtual ~BrowserPluginManagerFactory() {}
};

}  // namespace content

#endif  // CONTENT_BROWSER_BROWSER_PLUGIN_BROWSER_PLUGIN_MANAGER_FACTORY_H_
