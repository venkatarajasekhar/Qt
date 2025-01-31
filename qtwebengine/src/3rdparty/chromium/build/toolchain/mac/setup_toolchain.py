# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import stat
import sys

def CopyTool(source_path):
  """Copies the given tool to the current directory, including a warning not
  to edit it."""
  with open(source_path) as source_file:
    tool_source = source_file.readlines()

  # Add header and write it out to the current directory (which should be the
  # root build dir).
  out_path = 'gyp-mac-tool'
  with open(out_path, 'w') as tool_file:
    tool_file.write(''.join([tool_source[0],
                             '# Generated by setup_toolchain.py do not edit.\n']
                            + tool_source[1:]))
  st = os.stat(out_path)
  os.chmod(out_path, st.st_mode | stat.S_IEXEC)

# Find the tool source, it's the first argument, and copy it.
if len(sys.argv) != 2:
  print "Need one argument (mac_tool source path)."
  sys.exit(1)
CopyTool(sys.argv[1])
