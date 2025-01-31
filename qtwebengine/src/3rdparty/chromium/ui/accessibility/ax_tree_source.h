// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_AX_TREE_SOURCE_H_
#define UI_ACCESSIBILITY_AX_TREE_SOURCE_H_

#include <vector>

#include "ui/accessibility/ax_node_data.h"

namespace ui {

// An AXTreeSource is an abstract interface for a serializable
// accessibility tree. The tree may be in some other format or
// may be computed dynamically, but maintains the properties that
// it's a strict tree, it has a unique id for each node, and all
// of the accessibility information about a node can be serialized
// as an AXNodeData. This is the primary interface to use when
// an accessibility tree will be sent over an IPC before being
// consumed.
template<typename AXNodeSource>
class AXTreeSource {
 public:
  virtual ~AXTreeSource() {}

  // Get the root of the tree.
  virtual AXNodeSource GetRoot() const = 0;

  // Get a node by its id. If no node by that id exists in the tree, return a
  // null node, i.e. one that will return false if you call IsValid on it.
  virtual AXNodeSource GetFromId(int32 id) const = 0;

  // Return the id of a node. All ids must be positive integers.
  virtual int32 GetId(AXNodeSource node) const = 0;

  // Append all children of |node| to |out_children|.
  virtual void GetChildren(AXNodeSource node,
                           std::vector<AXNodeSource>* out_children) const = 0;

  // Get the parent of |node|.
  virtual AXNodeSource GetParent(AXNodeSource node) const = 0;

  // Returns true if |node| is valid, and false if it's a null pointer or a
  // node object representing the null pointer.
  virtual bool IsValid(AXNodeSource node) const = 0;

  // Returns true if two nodes are equal.
  virtual bool IsEqual(AXNodeSource node1,
                       AXNodeSource node2) const = 0;

  // Return a AXNodeSource representing null.
  virtual AXNodeSource GetNull() const = 0;

  // Serialize one node in the tree.
  virtual void SerializeNode(AXNodeSource node, AXNodeData* out_data) const = 0;

 protected:
  AXTreeSource() {}
};

}  // namespace ui

#endif  // UI_ACCESSIBILITY_AX_TREE_SOURCE_H_
