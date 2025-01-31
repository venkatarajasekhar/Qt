#!/usr/bin/env python

# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

# Searches for libraries or object files on the specified path and merges them
# them into a single library. Assumes ninja is used on all platforms.

import fnmatch
import os
import subprocess
import sys


def FindFiles(path, pattern):
  """Finds files matching |pattern| under |path|.

  Returns a list of file paths matching |pattern|, by walking the directory tree
  under |path|. Filenames containing the string 'do_not_use' or 'protoc' are
  excluded.

  Args:
    path: The root path for the search.
    pattern: A shell-style wildcard pattern to match filenames against.
        (e.g. '*.a')

  Returns:
    A list of file paths, relative to the current working directory.
  """
  files = []
  for root, _, filenames in os.walk(path):
    for filename in fnmatch.filter(filenames, pattern):
      if 'do_not_use' not in filename and 'protoc' not in filename:
        # We use the relative path here to avoid "argument list too long"
        # errors on Linux.
        files.append(os.path.relpath(os.path.join(root, filename)))
  return files


def main(argv):
  if len(argv) != 3:
    sys.stderr.write('Usage: ' + argv[0] + ' <search_path> <output_lib>\n')
    return 1

  search_path = os.path.normpath(argv[1])
  output_lib = os.path.normpath(argv[2])

  if not os.path.exists(search_path):
    sys.stderr.write('search_path does not exist: %s\n' % search_path)
    return 1

  if os.path.isfile(output_lib):
    os.remove(output_lib)

  if sys.platform.startswith('linux'):
    objects = FindFiles(search_path, '*.o')
    cmd = 'ar crs '
  elif sys.platform == 'darwin':
    objects = FindFiles(search_path, '*.a')
    cmd = 'libtool -static -v -o '
  elif sys.platform == 'win32':
    objects = FindFiles(search_path, '*.lib')
    cmd = 'lib /OUT:'
  else:
    sys.stderr.write('Platform not supported: %r\n\n' % sys.platform)
    return 1

  cmd += output_lib + ' ' + ' '.join(objects)
  print cmd
  subprocess.check_call(cmd, shell=True)
  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv))

