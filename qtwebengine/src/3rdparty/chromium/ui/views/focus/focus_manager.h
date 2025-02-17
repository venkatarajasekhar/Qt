// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_FOCUS_FOCUS_MANAGER_H_
#define UI_VIEWS_FOCUS_FOCUS_MANAGER_H_

#include <list>
#include <map>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/observer_list.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/accelerator_manager.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/views_export.h"

// The FocusManager class is used to handle focus traversal, store/restore
// focused views and handle keyboard accelerators.
//
// There are 2 types of focus:
// - the native focus, which is the focus that an gfx::NativeView has.
// - the view focus, which is the focus that a views::View has.
//
// Each native view must register with their Focus Manager so the focus manager
// gets notified when they are focused (and keeps track of the native focus) and
// as well so that the tab key events can be intercepted.
// They can provide when they register a View that is kept in synch in term of
// focus. This is used in NativeControl for example, where a View wraps an
// actual native window.
// This is already done for you if you subclass the NativeControl class or if
// you use the NativeViewHost class.
//
// When creating a top window (derived from views::Widget) that is not a child
// window, it creates and owns a FocusManager to manage the focus for itself and
// all its child windows.
//
// The FocusTraversable interface exposes the methods a class should implement
// in order to be able to be focus traversed when tab key is pressed.
// RootViews implement FocusTraversable.
// The FocusManager contains a top FocusTraversable instance, which is the top
// RootView.
//
// If you just use views, then the focus traversal is handled for you by the
// RootView. The default traversal order is the order in which the views have
// been added to their container. You can modify this order by using the View
// method SetNextFocusableView().
//
// If you are embedding a native view containing a nested RootView (for example
// by adding a NativeControl that contains a NativeWidgetWin as its native
// component), then you need to:
// - override the View::GetFocusTraversable() method in your outer component.
//   It should return the RootView of the inner component. This is used when
//   the focus traversal traverse down the focus hierarchy to enter the nested
//   RootView. In the example mentioned above, the NativeControl overrides
//   GetFocusTraversable() and returns hwnd_view_container_->GetRootView().
// - call Widget::SetFocusTraversableParent() on the nested RootView and point
//   it to the outer RootView. This is used when the focus goes out of the
//   nested RootView. In the example:
//   hwnd_view_container_->GetWidget()->SetFocusTraversableParent(
//      native_control->GetRootView());
// - call RootView::SetFocusTraversableParentView() on the nested RootView with
//   the parent view that directly contains the native window. This is needed
//   when traversing up from the nested RootView to know which view to start
//   with when going to the next/previous view.
//   In our example:
//   hwnd_view_container_->GetWidget()->SetFocusTraversableParent(
//      native_control);
//
// Note that FocusTraversable do not have to be RootViews: AccessibleToolbarView
// is FocusTraversable.

namespace ui {
class AcceleratorManager;
class AcceleratorTarget;
class EventHandler;
class KeyEvent;
}

namespace views {

class FocusManagerDelegate;
class FocusSearch;
class RootView;
class View;
class Widget;

// The FocusTraversable interface is used by components that want to process
// focus traversal events (due to Tab/Shift-Tab key events).
class VIEWS_EXPORT FocusTraversable {
 public:
  // Return a FocusSearch object that implements the algorithm to find
  // the next or previous focusable view.
  virtual FocusSearch* GetFocusSearch() = 0;

  // Should return the parent FocusTraversable.
  // The top RootView which is the top FocusTraversable returns NULL.
  virtual FocusTraversable* GetFocusTraversableParent() = 0;

  // This should return the View this FocusTraversable belongs to.
  // It is used when walking up the view hierarchy tree to find which view
  // should be used as the starting view for finding the next/previous view.
  virtual View* GetFocusTraversableParentView() = 0;

 protected:
  virtual ~FocusTraversable() {}
};

// This interface should be implemented by classes that want to be notified when
// the focus is about to change.  See the Add/RemoveFocusChangeListener methods.
class VIEWS_EXPORT FocusChangeListener {
 public:
  // No change to focus state has occurred yet when this function is called.
  virtual void OnWillChangeFocus(View* focused_before, View* focused_now) = 0;

  // Called after focus state has changed.
  virtual void OnDidChangeFocus(View* focused_before, View* focused_now) = 0;

 protected:
  virtual ~FocusChangeListener() {}
};

class VIEWS_EXPORT FocusManager {
 public:
  // The reason why the focus changed.
  enum FocusChangeReason {
    // The focus changed because the user traversed focusable views using
    // keys like Tab or Shift+Tab.
    kReasonFocusTraversal,

    // The focus changed due to restoring the focus.
    kReasonFocusRestore,

    // The focus changed due to a click or a shortcut to jump directly to
    // a particular view.
    kReasonDirectFocusChange
  };

  // TODO: use Direction in place of bool reverse throughout.
  enum Direction {
    kForward,
    kBackward
  };

  enum FocusCycleWrappingBehavior {
    kWrap,
    kNoWrap
  };

  FocusManager(Widget* widget, FocusManagerDelegate* delegate);
  virtual ~FocusManager();

  // Processes the passed key event for accelerators and keyboard traversal.
  // Returns false if the event has been consumed and should not be processed
  // further.
  bool OnKeyEvent(const ui::KeyEvent& event);

  // Returns true is the specified is part of the hierarchy of the window
  // associated with this FocusManager.
  bool ContainsView(View* view);

  // Advances the focus (backward if reverse is true).
  void AdvanceFocus(bool reverse);

  // The FocusManager keeps track of the focused view within a RootView.
  View* GetFocusedView() { return focused_view_; }
  const View* GetFocusedView() const { return focused_view_; }

  // Low-level methods to force the focus to change (and optionally provide
  // a reason). If the focus change should only happen if the view is
  // currenty focusable, enabled, and visible, call view->RequestFocus().
  void SetFocusedViewWithReason(View* view, FocusChangeReason reason);
  void SetFocusedView(View* view) {
    SetFocusedViewWithReason(view, kReasonDirectFocusChange);
  }

  // Get the reason why the focus most recently changed.
  FocusChangeReason focus_change_reason() const {
    return focus_change_reason_;
  }

  // Clears the focused view. The window associated with the top root view gets
  // the native focus (so we still get keyboard events).
  void ClearFocus();

  // Validates the focused view, clearing it if the window it belongs too is not
  // attached to the window hierarchy anymore.
  void ValidateFocusedView();

  // Stores the focused view. Used when the widget loses activation.
  // |clear_native_focus| indicates whether this should invoke ClearFocus().
  // Typically |true| should be passed in.
  void StoreFocusedView(bool clear_native_focus);

  // Restore the view saved with a previous call to StoreFocusedView(). Used
  // when the widget becomes active. Returns true when the previous view was
  // successfully refocused - otherwise false.
  bool RestoreFocusedView();

  // Sets the |view| to be restored when calling RestoreFocusView. This is used
  // to set where the focus should go on restoring a Window created without
  // focus being set.
  void SetStoredFocusView(View* view);

  // Returns the View that either currently has focus, or if no view has focus
  // the view that last had focus.
  View* GetStoredFocusView();

  // Clears the stored focused view.
  void ClearStoredFocusedView();

  // Returns true if in the process of changing the focused view.
  bool is_changing_focus() const { return is_changing_focus_; }

  // Changes the text input focus to |view->GetTextInputClient()| iff |view|
  // is focused.  Views must call this method when their internal
  // TextInputClient instance changes.
  void OnTextInputClientChanged(View* view);

  // Moves the text input focus into/out from |view|.
  void FocusTextInputClient(View* view);
  void BlurTextInputClient(View* view);

  // Disable shortcut handling.
  static void set_shortcut_handling_suspended(bool suspended) {
    shortcut_handling_suspended_ = suspended;
  }
  // Returns whether shortcut handling is currently suspended.
  bool shortcut_handling_suspended() { return shortcut_handling_suspended_; }

  // Register a keyboard accelerator for the specified target. If multiple
  // targets are registered for an accelerator, a target registered later has
  // higher priority.
  // |accelerator| is the accelerator to register.
  // |priority| denotes the priority of the handler.
  // NOTE: In almost all cases, you should specify kNormalPriority for this
  // parameter. Setting it to kHighPriority prevents Chrome from sending the
  // shortcut to the webpage if the renderer has focus, which is not desirable
  // except for very isolated cases.
  // |target| is the AcceleratorTarget that handles the event once the
  // accelerator is pressed.
  // Note that we are currently limited to accelerators that are either:
  // - a key combination including Ctrl or Alt
  // - the escape key
  // - the enter key
  // - any F key (F1, F2, F3 ...)
  // - any browser specific keys (as available on special keyboards)
  void RegisterAccelerator(const ui::Accelerator& accelerator,
                           ui::AcceleratorManager::HandlerPriority priority,
                           ui::AcceleratorTarget* target);

  // Unregister the specified keyboard accelerator for the specified target.
  void UnregisterAccelerator(const ui::Accelerator& accelerator,
                             ui::AcceleratorTarget* target);

  // Unregister all keyboard accelerator for the specified target.
  void UnregisterAccelerators(ui::AcceleratorTarget* target);

  // Activate the target associated with the specified accelerator.
  // First, AcceleratorPressed handler of the most recently registered target
  // is called, and if that handler processes the event (i.e. returns true),
  // this method immediately returns. If not, we do the same thing on the next
  // target, and so on.
  // Returns true if an accelerator was activated.
  bool ProcessAccelerator(const ui::Accelerator& accelerator);

  // Resets menu key state if |event| is not menu key release.
  // This is effective only on x11.
  void MaybeResetMenuKeyState(const ui::KeyEvent& key);

  // Called by a RootView when a view within its hierarchy is removed
  // from its parent. This will only be called by a RootView in a
  // hierarchy of Widgets that this FocusManager is attached to the
  // parent Widget of.
  void ViewRemoved(View* removed);

  // Adds/removes a listener.  The FocusChangeListener is notified every time
  // the focused view is about to change.
  void AddFocusChangeListener(FocusChangeListener* listener);
  void RemoveFocusChangeListener(FocusChangeListener* listener);

  // Returns the AcceleratorTarget that should be activated for the specified
  // keyboard accelerator, or NULL if no view is registered for that keyboard
  // accelerator.
  ui::AcceleratorTarget* GetCurrentTargetForAccelerator(
      const ui::Accelerator& accelertor) const;

  // Whether the given |accelerator| has a priority handler associated with it.
  bool HasPriorityHandler(const ui::Accelerator& accelerator) const;

  // Clears the native view having the focus.
  virtual void ClearNativeFocus();

  // Focuses the next keyboard-accessible pane, taken from the list of
  // views returned by WidgetDelegate::GetAccessiblePanes(). If there are
  // no panes, the widget's root view is treated as a single pane.
  // A keyboard-accessible pane should subclass from AccessiblePaneView in
  // order to trap keyboard focus within that pane. If |wrap| is kWrap,
  // it keeps cycling within this widget, otherwise it returns false after
  // reaching the last pane so that focus can cycle to another widget.
  bool RotatePaneFocus(Direction direction, FocusCycleWrappingBehavior wrap);

  // Convenience method that returns true if the passed |key_event| should
  // trigger tab traversal (if it is a TAB key press with or without SHIFT
  // pressed).
  static bool IsTabTraversalKeyEvent(const ui::KeyEvent& key_event);

  // Sets whether arrow key traversal is enabled. When enabled, right/down key
  // behaves like tab and left/up key behaves like shift-tab. Note when this
  // is enabled, the arrow key movement within grouped views are disabled.
  static void set_arrow_key_traversal_enabled(bool enabled) {
    arrow_key_traversal_enabled_ = enabled;
  }
  // Returns whether arrow key traversal is enabled.
  static bool arrow_key_traversal_enabled() {
    return arrow_key_traversal_enabled_;
  }

  // Returns the next focusable view. Traversal starts at |starting_view|. If
  // |starting_view| is NULL |starting_widget| is consuled to determine which
  // Widget to start from. See
  // WidgetDelegate::ShouldAdvanceFocusToTopLevelWidget() for details. If both
  // |starting_view| and |starting_widget| are NULL, traversal starts at
  // |widget_|.
  View* GetNextFocusableView(View* starting_view,
                             Widget* starting_widget,
                             bool reverse,
                             bool dont_loop);

 private:
  // Returns the focusable view found in the FocusTraversable specified starting
  // at the specified view. This traverses down along the FocusTraversable
  // hierarchy.
  // Returns NULL if no focusable view were found.
  View* FindFocusableView(FocusTraversable* focus_traversable,
                          View* starting_view,
                          bool reverse);

  // Process arrow key traversal. Returns true if the event has been consumed
  // and should not be processed further.
  bool ProcessArrowKeyTraversal(const ui::KeyEvent& event);

  // Keeps track of whether shortcut handling is currently suspended.
  static bool shortcut_handling_suspended_;

  // Whether arrow key traversal is enabled.
  static bool arrow_key_traversal_enabled_;

  // The top-level Widget this FocusManager is associated with.
  Widget* widget_;

  // The object which handles an accelerator when |accelerator_manager_| doesn't
  // handle it.
  scoped_ptr<FocusManagerDelegate> delegate_;

  // The view that currently is focused.
  View* focused_view_;

  // The AcceleratorManager this FocusManager is associated with.
  scoped_ptr<ui::AcceleratorManager> accelerator_manager_;

  // The storage id used in the ViewStorage to store/restore the view that last
  // had focus.
  int stored_focused_view_storage_id_;

  // The reason why the focus most recently changed.
  FocusChangeReason focus_change_reason_;

  // The list of registered FocusChange listeners.
  ObserverList<FocusChangeListener, true> focus_change_listeners_;

  // See description above getter.
  bool is_changing_focus_;

  DISALLOW_COPY_AND_ASSIGN(FocusManager);
};

}  // namespace views

#endif  // UI_VIEWS_FOCUS_FOCUS_MANAGER_H_
