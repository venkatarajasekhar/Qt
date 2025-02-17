// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_aura/desktop_drop_target_win.h"

#include "base/win/win_util.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/drop_target_event.h"
#include "ui/base/dragdrop/os_exchange_data_provider_win.h"
#include "ui/events/event_constants.h"
#include "ui/wm/public/drag_drop_client.h"
#include "ui/wm/public/drag_drop_delegate.h"

using aura::client::DragDropDelegate;
using ui::OSExchangeData;
using ui::OSExchangeDataProviderWin;

namespace views {

DesktopDropTargetWin::DesktopDropTargetWin(aura::Window* root_window,
                                           HWND window)
    : ui::DropTargetWin(window),
      root_window_(root_window),
      target_window_(NULL) {
}

DesktopDropTargetWin::~DesktopDropTargetWin() {
  if (target_window_)
    target_window_->RemoveObserver(this);
}

DWORD DesktopDropTargetWin::OnDragEnter(IDataObject* data_object,
                                        DWORD key_state,
                                        POINT position,
                                        DWORD effect) {
  scoped_ptr<OSExchangeData> data;
  scoped_ptr<ui::DropTargetEvent> event;
  DragDropDelegate* delegate;
  // Translate will call OnDragEntered.
  Translate(data_object, key_state, position, effect, &data, &event, &delegate);
  return ui::DragDropTypes::DragOperationToDropEffect(
      ui::DragDropTypes::DRAG_NONE);
}

DWORD DesktopDropTargetWin::OnDragOver(IDataObject* data_object,
                                       DWORD key_state,
                                       POINT position,
                                       DWORD effect) {
  int drag_operation = ui::DragDropTypes::DRAG_NONE;
  scoped_ptr<OSExchangeData> data;
  scoped_ptr<ui::DropTargetEvent> event;
  DragDropDelegate* delegate;
  Translate(data_object, key_state, position, effect, &data, &event, &delegate);
  if (delegate)
    drag_operation = delegate->OnDragUpdated(*event);
  return ui::DragDropTypes::DragOperationToDropEffect(drag_operation);
}

void DesktopDropTargetWin::OnDragLeave(IDataObject* data_object) {
  NotifyDragLeave();
}

DWORD DesktopDropTargetWin::OnDrop(IDataObject* data_object,
                                   DWORD key_state,
                                   POINT position,
                                   DWORD effect) {
  int drag_operation = ui::DragDropTypes::DRAG_NONE;
  scoped_ptr<OSExchangeData> data;
  scoped_ptr<ui::DropTargetEvent> event;
  DragDropDelegate* delegate;
  Translate(data_object, key_state, position, effect, &data, &event, &delegate);
  if (delegate)
    drag_operation = delegate->OnPerformDrop(*event);
  if (target_window_) {
    target_window_->RemoveObserver(this);
    target_window_ = NULL;
  }
  return ui::DragDropTypes::DragOperationToDropEffect(drag_operation);
}

void DesktopDropTargetWin::OnWindowDestroyed(aura::Window* window) {
  DCHECK(window == target_window_);
  target_window_ = NULL;
}

void DesktopDropTargetWin::Translate(
    IDataObject* data_object,
    DWORD key_state,
    POINT position,
    DWORD effect,
    scoped_ptr<OSExchangeData>* data,
    scoped_ptr<ui::DropTargetEvent>* event,
    DragDropDelegate** delegate) {
  gfx::Point location(position.x, position.y);
  gfx::Point root_location = location;
  root_window_->GetHost()->ConvertPointFromNativeScreen(
      &root_location);
  aura::Window* target_window =
      root_window_->GetEventHandlerForPoint(root_location);
  bool target_window_changed = false;
  if (target_window != target_window_) {
    if (target_window_)
      NotifyDragLeave();
    target_window_ = target_window;
    if (target_window_)
      target_window_->AddObserver(this);
    target_window_changed = true;
  }
  *delegate = NULL;
  if (!target_window_)
    return;
  *delegate = aura::client::GetDragDropDelegate(target_window_);
  if (!*delegate)
    return;

  data->reset(new OSExchangeData(new OSExchangeDataProviderWin(data_object)));
  location = root_location;
  aura::Window::ConvertPointToTarget(root_window_, target_window_, &location);
  event->reset(new ui::DropTargetEvent(
      *(data->get()),
      location,
      root_location,
      ui::DragDropTypes::DropEffectToDragOperation(effect)));
  int flags = 0;
  flags |= base::win::IsAltPressed() ? ui::EF_ALT_DOWN : ui::EF_NONE;
  flags |= base::win::IsShiftPressed() ? ui::EF_SHIFT_DOWN : ui::EF_NONE;
  flags |= base::win::IsCtrlPressed() ? ui::EF_CONTROL_DOWN : ui::EF_NONE;
  (*event)->set_flags(flags);
  if (target_window_changed)
    (*delegate)->OnDragEntered(*event->get());
}

void DesktopDropTargetWin::NotifyDragLeave() {
  if (!target_window_)
    return;
  DragDropDelegate* delegate =
      aura::client::GetDragDropDelegate(target_window_);
  if (delegate)
    delegate->OnDragExited();
  target_window_->RemoveObserver(this);
  target_window_ = NULL;
}

}  // namespace views
