#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
A simple utility function to merge data pack files into a single data pack. See
http://dev.chromium.org/developers/design-documents/linuxresourcesandlocalizedstrings
for details about the file format.
"""

import optparse
import os
import sys

if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

import grit.format.data_pack


def main(argv):
  parser = optparse.OptionParser('usage: %prog [options] <output_filename>'
                                 '<input_file1> [input_file2] ...')
  parser.add_option('--whitelist', action='store', dest='whitelist',
                    default=None, help='Full path to the whitelist used to'
                    'filter output pak file resource IDs')
  options, file_paths = parser.parse_args(argv)

  if len(file_paths) < 2:
    parser.error('Please specify output and at least one input filenames')

  grit.format.data_pack.RePack(file_paths[0], file_paths[1:],
                               whitelist_file=options.whitelist)

if '__main__' == __name__:
  main(sys.argv[1:])
