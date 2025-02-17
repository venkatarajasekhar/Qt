#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" Output file objects for generator. """

import difflib
import os
import time
import sys

from idl_log import ErrOut, InfoOut, WarnOut
from idl_option import GetOption, Option, ParseOptions
from stat import *

Option('diff', 'Generate a DIFF when saving the file.')


#
# IDLOutFile
#
# IDLOutFile provides a temporary output file.  By default, the object will
# not write the output if the file already exists, and matches what will be
# written.  This prevents the timestamp from changing to optimize cases where
# the output files are used by a timestamp dependent build system
#
class IDLOutFile(object):
  def __init__(self, filename, always_write = False, create_dir = True):
    self.filename = filename
    self.always_write = always_write
    self.create_dir = create_dir
    self.outlist = []
    self.open = True

  # Compare the old text to the current list of output lines.
  def IsEquivalent_(self, oldtext):
    if not oldtext: return False

    oldlines = oldtext.split('\n')
    curlines = (''.join(self.outlist)).split('\n')

    # If number of lines don't match, it's a mismatch
    if len(oldlines) != len(curlines):
      return False

    for index in range(len(oldlines)):
      oldline = oldlines[index]
      curline = curlines[index]

      if oldline == curline: continue

      curwords = curline.split()
      oldwords = oldline.split()

      # Unmatched lines must be the same length
      if len(curwords) != len(oldwords):
        return False

      # If it's not a comment then it's a mismatch
      if curwords[0] not in ['*', '/*', '//']:
        return False

      # Ignore changes to the Copyright year which is autogenerated
      # /* Copyright (c) 2011 The Chromium Authors. All rights reserved.
      if len(curwords) > 4 and curwords[1] == 'Copyright':
        if curwords[4:] == oldwords[4:]: continue

      # Ignore changes to auto generation timestamp when line unwrapped
      # // From FILENAME.idl modified DAY MON DATE TIME YEAR.
      # /* From FILENAME.idl modified DAY MON DATE TIME YEAR. */
      if len(curwords) > 8 and curwords[1] == 'From':
        if curwords[0:4] == oldwords[0:4]: continue

      # Ignore changes to auto generation timestamp when line is wrapped
      # * modified DAY MON DATE TIME YEAR.
      if len(curwords) > 6 and curwords[1] == 'modified':
        continue

      return False
    return True

  # Return the file name
  def Filename(self):
    return self.filename

  # Append to the output if the file is still open
  def Write(self, string):
    if not self.open:
      raise RuntimeError('Could not write to closed file %s.' % self.filename)
    self.outlist.append(string)

  # Close the file, flushing it to disk
  def Close(self):
    filename = os.path.realpath(self.filename)
    self.open = False
    outtext = ''.join(self.outlist)
    oldtext = ''

    if not self.always_write:
      if os.path.isfile(filename):
        oldtext = open(filename, 'rb').read()
        if self.IsEquivalent_(oldtext):
          if GetOption('verbose'):
            InfoOut.Log('Output %s unchanged.' % self.filename)
          return False

    if GetOption('diff'):
      for line in difflib.unified_diff(oldtext.split('\n'), outtext.split('\n'),
                                       'OLD ' + self.filename,
                                       'NEW ' + self.filename,
                                       n=1, lineterm=''):
        ErrOut.Log(line)

    try:
      # If the directory does not exit, try to create it, if we fail, we
      # still get the exception when the file is openned.
      basepath, leafname = os.path.split(filename)
      if basepath and not os.path.isdir(basepath) and self.create_dir:
        InfoOut.Log('Creating directory: %s\n' % basepath)
        os.makedirs(basepath)

      if not GetOption('test'):
        outfile = open(filename, 'wb')
        outfile.write(outtext)
        InfoOut.Log('Output %s written.' % self.filename)
      return True

    except IOError as (errno, strerror):
      ErrOut.Log("I/O error(%d): %s" % (errno, strerror))
    except:
      ErrOut.Log("Unexpected error: %s" % sys.exc_info()[0])
      raise

    return False


def TestFile(name, stringlist, force, update):
  errors = 0

  # Get the old timestamp
  if os.path.exists(name):
    old_time = os.stat(filename)[ST_MTIME]
  else:
    old_time = 'NONE'

  # Create the file and write to it
  out = IDLOutFile(filename, force)
  for item in stringlist:
    out.Write(item)

  # We wait for flush to force the timestamp to change
  time.sleep(2)

  wrote = out.Close()
  cur_time = os.stat(filename)[ST_MTIME]
  if update:
    if not wrote:
      ErrOut.Log('Failed to write output %s.' % filename)
      return 1
    if cur_time == old_time:
      ErrOut.Log('Failed to update timestamp for %s.' % filename)
      return 1
  else:
    if wrote:
      ErrOut.Log('Should not have writen output %s.' % filename)
      return 1
    if cur_time != old_time:
      ErrOut.Log('Should not have modified timestamp for %s.' % filename)
      return 1
  return 0


def main():
  errors = 0
  stringlist = ['Test', 'Testing\n', 'Test']
  filename = 'outtest.txt'

  # Test forcibly writing a file
  errors += TestFile(filename, stringlist, force=True, update=True)

  # Test conditionally writing the file skipping
  errors += TestFile(filename, stringlist, force=False, update=False)

  # Test conditionally writing the file updating
  errors += TestFile(filename, stringlist + ['X'], force=False, update=True)

  # Clean up file
  os.remove(filename)
  if not errors: InfoOut.Log('All tests pass.')
  return errors


if __name__ == '__main__':
  sys.exit(main())
