#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Item formatters for RC headers.
'''

from grit import exception
from grit import util
from grit.extern import FP


def Format(root, lang='en', output_dir='.'):
  yield '''\
// This file is automatically generated by GRIT. Do not edit.

#pragma once
'''
  # Check for emit nodes under the rc_header. If any emit node
  # is present, we assume it means the GRD file wants to override
  # the default header, with no includes.
  default_includes = ['#include <atlres.h>', '']
  emit_lines = []
  for output_node in root.GetOutputFiles():
    if output_node.GetType() == 'rc_header':
      for child in output_node.children:
        if child.name == 'emit' and child.attrs['emit_type'] == 'prepend':
          emit_lines.append(child.GetCdata())
  for line in emit_lines or default_includes:
    yield line + '\n'

  for line in FormatDefines(root, root.ShouldOutputAllResourceDefines(),
                            root.GetRcHeaderFormat()):
    yield line


def FormatDefines(root, output_all_resource_defines=True,
                  rc_header_format=None):
  '''Yields #define SYMBOL 1234 lines.

  Args:
    root: A GritNode.
    output_all_resource_defines: If False, output only the symbols used in the
      current output configuration.
  '''
  from grit.node import message
  tids = GetIds(root)

  if output_all_resource_defines:
    items = root.Preorder()
  else:
    items = root.ActiveDescendants()

  if not rc_header_format:
    rc_header_format = "#define {textual_id} {numeric_id}"
  rc_header_format += "\n"
  seen = set()
  for item in items:
    if not isinstance(item, message.MessageNode):
      with item:
        for tid in item.GetTextualIds():
          if tid in tids and tid not in seen:
            seen.add(tid)
            yield rc_header_format.format(textual_id=tid,numeric_id=tids[tid])

  # Temporarily mimic old behavior: MessageNodes were only output if active,
  # even with output_all_resource_defines set. TODO(benrg): Remove this after
  # fixing problems in the Chrome tree.
  for item in root.ActiveDescendants():
    if isinstance(item, message.MessageNode):
      with item:
        for tid in item.GetTextualIds():
          if tid in tids and tid not in seen:
            seen.add(tid)
            yield rc_header_format.format(textual_id=tid,numeric_id=tids[tid])


_cached_ids = {}


def GetIds(root):
  '''Return a dictionary mapping textual ids to numeric ids for the given tree.

  Args:
    root: A GritNode.
  '''
  # TODO(benrg): Since other formatters use this, it might make sense to move it
  # and _ComputeIds to GritNode and store the cached ids as an attribute. On the
  # other hand, GritNode has too much random stuff already.
  if root not in _cached_ids:
    _cached_ids[root] = _ComputeIds(root)
  return _cached_ids[root]


def _ComputeIds(root):
  from grit.node import empty, include, message, misc, structure

  ids = {}  # Maps numeric id to textual id
  tids = {}  # Maps textual id to numeric id
  id_reasons = {}  # Maps numeric id to text id and a human-readable explanation
  group = None
  last_id = None

  for item in root:
    if isinstance(item, empty.GroupingNode):
      # Note: this won't work if any GroupingNode can be contained inside
      # another.
      group = item
      last_id = None
      continue

    assert not item.GetTextualIds() or isinstance(item,
        (include.IncludeNode, message.MessageNode,
         misc.IdentifierNode, structure.StructureNode))

    # Resources that use the RES protocol don't need
    # any numerical ids generated, so we skip them altogether.
    # This is accomplished by setting the flag 'generateid' to false
    # in the GRD file.
    if item.attrs.get('generateid', 'true') == 'false':
      continue

    for tid in item.GetTextualIds():
      if util.SYSTEM_IDENTIFIERS.match(tid):
        # Don't emit a new ID for predefined IDs
        continue

      if tid in tids:
        continue

      # Some identifier nodes can provide their own id,
      # and we use that id in the generated header in that case.
      if hasattr(item, 'GetId') and item.GetId():
        id = long(item.GetId())
        reason = 'returned by GetId() method'

      elif ('offset' in item.attrs and group and
            group.attrs.get('first_id', '') != ''):
         offset_text = item.attrs['offset']
         parent_text = group.attrs['first_id']

         try:
          offset_id = long(offset_text)
         except ValueError:
          offset_id = tids[offset_text]

         try:
          parent_id = long(parent_text)
         except ValueError:
          parent_id = tids[parent_text]

         id = parent_id + offset_id
         reason = 'first_id %d + offset %d' % (parent_id, offset_id)

      # We try to allocate IDs sequentially for blocks of items that might
      # be related, for instance strings in a stringtable (as their IDs might be
      # used e.g. as IDs for some radio buttons, in which case the IDs must
      # be sequential).
      #
      # We do this by having the first item in a section store its computed ID
      # (computed from a fingerprint) in its parent object.  Subsequent children
      # of the same parent will then try to get IDs that sequentially follow
      # the currently stored ID (on the parent) and increment it.
      elif last_id is None:
        # First check if the starting ID is explicitly specified by the parent.
        if group and group.attrs.get('first_id', '') != '':
          id = long(group.attrs['first_id'])
          reason = "from parent's first_id attribute"
        else:
          # Automatically generate the ID based on the first clique from the
          # first child of the first child node of our parent (i.e. when we
          # first get to this location in the code).

          # According to
          # http://msdn.microsoft.com/en-us/library/t2zechd4(VS.71).aspx
          # the safe usable range for resource IDs in Windows is from decimal
          # 101 to 0x7FFF.

          id = FP.UnsignedFingerPrint(tid)
          id = id % (0x7FFF - 101) + 101
          reason = 'chosen by random fingerprint -- use first_id to override'

        last_id = id
      else:
        id = last_id = last_id + 1
        reason = 'sequentially assigned'

      reason = "%s (%s)" % (tid, reason)
      # Don't fail when 'offset' is specified, as the base and the 0th
      # offset will have the same ID.
      if id in id_reasons and not 'offset' in item.attrs:
        raise exception.IdRangeOverlap('ID %d was assigned to both %s and %s.'
                                       % (id, id_reasons[id], reason))

      if id < 101:
        print ('WARNING: Numeric resource IDs should be greater than 100 to\n'
               'avoid conflicts with system-defined resource IDs.')

      ids[id] = tid
      tids[tid] = id
      id_reasons[id] = reason

  return tids
