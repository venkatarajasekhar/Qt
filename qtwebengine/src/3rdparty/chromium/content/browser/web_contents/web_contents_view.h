// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_H_
#define CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_H_

#include <string>

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"

namespace content {
class RenderViewHost;
class RenderWidgetHost;
class RenderWidgetHostViewBase;
struct DropData;

// The WebContentsView is an interface that is implemented by the platform-
// dependent web contents views. The WebContents uses this interface to talk to
// them.
class WebContentsView {
 public:
  virtual ~WebContentsView() {}

  // Returns the native widget that contains the contents of the tab.
  virtual gfx::NativeView GetNativeView() const = 0;

  // Returns the native widget with the main content of the tab (i.e. the main
  // render view host, though there may be many popups in the tab as children of
  // the container).
  virtual gfx::NativeView GetContentNativeView() const = 0;

  // Returns the outermost native view. This will be used as the parent for
  // dialog boxes.
  virtual gfx::NativeWindow GetTopLevelNativeWindow() const = 0;

  // Computes the rectangle for the native widget that contains the contents of
  // the tab in the screen coordinate system.
  virtual void GetContainerBounds(gfx::Rect* out) const = 0;

  // TODO(brettw) this is a hack. It's used in two places at the time of this
  // writing: (1) when render view hosts switch, we need to size the replaced
  // one to be correct, since it wouldn't have known about sizes that happened
  // while it was hidden; (2) in constrained windows.
  //
  // (1) will be fixed once interstitials are cleaned up. (2) seems like it
  // should be cleaned up or done some other way, since this works for normal
  // WebContents without the special code.
  virtual void SizeContents(const gfx::Size& size) = 0;

  // Sets focus to the native widget for this tab.
  virtual void Focus() = 0;

  // Sets focus to the appropriate element when the WebContents is shown the
  // first time.
  virtual void SetInitialFocus() = 0;

  // Stores the currently focused view.
  virtual void StoreFocus() = 0;

  // Restores focus to the last focus view. If StoreFocus has not yet been
  // invoked, SetInitialFocus is invoked.
  virtual void RestoreFocus() = 0;

  // Returns the current drop data, if any.
  virtual DropData* GetDropData() const = 0;

  // Get the bounds of the View, relative to the parent.
  virtual gfx::Rect GetViewBounds() const = 0;

  virtual void CreateView(
      const gfx::Size& initial_size, gfx::NativeView context) = 0;

  // Sets up the View that holds the rendered web page, receives messages for
  // it and contains page plugins. The host view should be sized to the current
  // size of the WebContents.
  virtual RenderWidgetHostViewBase* CreateViewForWidget(
      RenderWidgetHost* render_widget_host) = 0;

  // Creates a new View that holds a popup and receives messages for it.
  virtual RenderWidgetHostViewBase* CreateViewForPopupWidget(
      RenderWidgetHost* render_widget_host) = 0;

  // Sets the page title for the native widgets corresponding to the view. This
  // is not strictly necessary and isn't expected to be displayed anywhere, but
  // can aid certain debugging tools such as Spy++ on Windows where you are
  // trying to find a specific window.
  virtual void SetPageTitle(const base::string16& title) = 0;

  // Invoked when the WebContents is notified that the RenderView has been
  // fully created.
  virtual void RenderViewCreated(RenderViewHost* host) = 0;

  // Invoked when the WebContents is notified that the RenderView has been
  // swapped in.
  virtual void RenderViewSwappedIn(RenderViewHost* host) = 0;

  // Invoked to enable/disable overscroll gesture navigation.
  virtual void SetOverscrollControllerEnabled(bool enabled) = 0;

#if defined(OS_MACOSX)
  // The web contents view assumes that its view will never be overlapped by
  // another view (either partially or fully). This allows it to perform
  // optimizations. If the view is in a view hierarchy where it might be
  // overlapped by another view, notify the view by calling this with |true|.
  virtual void SetAllowOverlappingViews(bool overlapping) = 0;

  // Returns true if overlapping views are allowed, false otherwise.
  virtual bool GetAllowOverlappingViews() const = 0;

  // To draw two overlapping web contents view, the underlaying one should
  // know about the overlaying one. Caller must ensure that |overlay| exists
  // until |RemoveOverlayView| is called.
  virtual void SetOverlayView(WebContentsView* overlay,
                              const gfx::Point& offset) = 0;

  // Removes the previously set overlay view.
  virtual void RemoveOverlayView() = 0;

  // If we close the tab while a UI control is in an event-tracking
  // loop, the control may message freed objects and crash.
  // WebContents::Close() calls IsEventTracking(), and if it returns
  // true CloseTabAfterEventTracking() is called and the close is not
  // completed.
  virtual bool IsEventTracking() const = 0;
  virtual void CloseTabAfterEventTracking() = 0;
#endif
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_H_
