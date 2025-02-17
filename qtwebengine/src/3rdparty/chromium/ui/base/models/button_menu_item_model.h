// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_MODELS_BUTTON_MENU_ITEM_MODEL_H_
#define UI_BASE_MODELS_BUTTON_MENU_ITEM_MODEL_H_

#include <vector>

#include "base/strings/string16.h"
#include "ui/base/ui_base_export.h"

namespace ui {

// A model representing the rows of buttons that should be inserted in a button
// containing menu item.
class UI_BASE_EXPORT ButtonMenuItemModel {
 public:
  // Types of buttons.
  enum ButtonType {
    TYPE_SPACE,
    TYPE_BUTTON,
    TYPE_BUTTON_LABEL
  };

  class UI_BASE_EXPORT Delegate {
   public:
    // Some command ids have labels that change over time.
    virtual bool IsItemForCommandIdDynamic(int command_id) const;
    virtual base::string16 GetLabelForCommandId(int command_id) const;

    // Performs the action associated with the specified command id.
    virtual void ExecuteCommand(int command_id, int event_flags) = 0;
    virtual bool IsCommandIdEnabled(int command_id) const;
    virtual bool DoesCommandIdDismissMenu(int command_id) const;

   protected:
    virtual ~Delegate() {}
  };

  ButtonMenuItemModel(int string_id, ButtonMenuItemModel::Delegate* delegate);
  ~ButtonMenuItemModel();

  // Adds a button that will emit |command_id|. All buttons created through
  // this method will have the same size, based on the largest button.
  void AddGroupItemWithStringId(int command_id, int string_id);

  // Adds a button that has an icon instead of a label.
  void AddItemWithImage(int command_id, int icon_idr);

  // Adds a non-clickable button with a desensitized label that doesn't do
  // anything. Usually combined with IsItemForCommandIdDynamic() to add
  // information.
  void AddButtonLabel(int command_id, int string_id);

  // Adds a small horizontal space.
  void AddSpace();

  // Returns the number of items for iteration.
  int GetItemCount() const;

  // Returns what kind of item is at |index|.
  ButtonType GetTypeAt(int index) const;

  // Changes a position into a command ID.
  int GetCommandIdAt(int index) const;

  // Whether the label for item |index| changes.
  bool IsItemDynamicAt(int index) const;

  // Returns the current label value for the button at |index|.
  base::string16 GetLabelAt(int index) const;

  // If the button at |index| should have an icon instead, returns true and
  // sets the IDR |icon|.
  bool GetIconAt(int index, int* icon) const;

  // If the button at |index| should have its size equalized along with all
  // other items that have their PartOfGroup bit set.
  bool PartOfGroup(int index) const;

  // Called from implementations.
  void ActivatedCommand(int command_id);

  // Returns the enabled state of the button at |index|.
  bool IsEnabledAt(int index) const;

  // Returns whether clicking on the button at |index| dismisses the menu.
  bool DismissesMenuAt(int index) const;

  // Returns the enabled state of the command specified by |command_id|.
  bool IsCommandIdEnabled(int command_id) const;

  // Returns whether clicking on |command_id| dismisses the menu.
  bool DoesCommandIdDismissMenu(int command_id) const;

  const base::string16& label() const { return item_label_; }

 private:
  // The non-clickable label to the left of the buttons.
  base::string16 item_label_;

  struct Item;
  std::vector<Item> items_;

  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(ButtonMenuItemModel);
};

}  // namespace ui

#endif  // UI_BASE_MODELS_BUTTON_MENU_ITEM_MODEL_H_
