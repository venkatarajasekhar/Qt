#!/usr/bin/python

"""
Copyright 2014 Google Inc.

Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.

Run all unittests within this directory tree, recursing into subdirectories.

TODO(epoger): Launch this automatically on the housekeeper bot, but first make
sure it works properly after having been checked out (from both git and svn)
"""

import os
import unittest


def main():
  suite = unittest.TestLoader().discover(os.path.dirname(__file__),
                                         pattern='*_test.py')
  results = unittest.TextTestRunner(verbosity=2).run(suite)
  print repr(results)
  if not results.wasSuccessful():
    raise Exception('failed one or more unittests')

if __name__ == '__main__':
  main()
