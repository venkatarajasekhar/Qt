#!/usr/bin/env python

# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

''' Generates a deps.js file based on an input list of javascript files using
Closure style provide/require calls.
'''

import optparse
import os
import sys

from jsbundler import PathRewriter

_SCRIPT_DIR = os.path.realpath(os.path.dirname(__file__))
_CHROME_SOURCE = os.path.realpath(
    os.path.join(_SCRIPT_DIR, *[os.path.pardir] * 6))
sys.path.insert(0, os.path.join(
    _CHROME_SOURCE, ('chrome/third_party/chromevox/third_party/' +
                     'closure-library/closure/bin/build')))
import source


def main():
  parser = optparse.OptionParser(description=__doc__)
  parser.add_option('-w', '--rewrite_prefix', action='append', default=[],
                    dest='prefix_map', metavar='SPEC',
                    help=('Two path prefixes, separated by colons ' +
                          'specifying that a file whose (relative) path ' +
                          'name starts with the first prefix should have ' +
                          'that prefix replaced by the second prefix to ' +
                          'form a path relative to the output directory. ' +
                          'The resulting path is used in the deps mapping ' +
                          'file path to a list of provided and required ' +
                          'namespaces.'))
  parser.add_option('-o', '--output_file', action='store', default=[],
                    metavar='SPEC',
                    help=('Where to output the generated deps file.'))
  options, args = parser.parse_args()

  path_rewriter = PathRewriter(options.prefix_map)

  # Write the generated deps file.
  with open(options.output_file, 'w') as output:
    for path in args:
      js_deps = source.Source(source.GetFileContents(path))
      path = path_rewriter.RewritePath(path)
      line = 'goog.addDependency(\'%s\', %s, %s);\n' % (
          path, sorted(js_deps.provides), sorted(js_deps.requires))
      output.write(line)


if __name__ == '__main__':
  main()
