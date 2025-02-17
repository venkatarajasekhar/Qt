#!/usr/bin/python

# Copyright 2014 Google Inc.
#
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Modified version of gyp_skia, used by gyp_to_android.py to generate Android.mk
"""

import os
import sys

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))

# Unlike gyp_skia, this file is nested deep inside Skia. Find Skia's trunk dir.
# This line depends on the fact that the script is three levels deep
# (specifically, it is in platform_tools/android/gyp_gen).
SKIA_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, os.pardir, os.pardir,
                                         os.pardir))
DIR_CONTENTS = os.listdir(SKIA_DIR)
assert 'gyp' in DIR_CONTENTS

# Directory within which we can find the gyp source.
gyp_source_dir = os.path.join(SKIA_DIR, 'third_party', 'externals', 'gyp')
if not os.path.exists(gyp_source_dir):
  # In an Android tree, there is no third_party/externals/gyp, which would
  # require running gclient sync. Use chromium's instead.
  gyp_source_dir = os.path.join(SKIA_DIR, os.pardir, 'chromium_org', 'tools',
                                'gyp')

assert os.path.exists(gyp_source_dir)

# Ensure we import our current gyp source's module, not any version
# pre-installed in your PYTHONPATH.
sys.path.insert(0, os.path.join(gyp_source_dir, 'pylib'))

import gyp

def main(target_dir, target_file, skia_arch_type, have_neon):
  """Create gypd files based on target_file.

  Args:
    target_dir: Directory containing all gyp files, including common.gypi
    target_file: Gyp file to start on. Other files within target_dir will
      be read if target_file depends on them.
    skia_arch_type: Target architecture to pass to gyp.
    have_neon: Whether to generate files including neon optimizations.
      Only meaningful if skia_arch_type is 'arm'.

  Returns:
    path: Path to root gypd file created by running gyp.
  """
  # Set GYP_DEFINES for building for the android framework.
  gyp_defines = ('skia_android_framework=1 OS=android skia_arch_type=%s '
                 % skia_arch_type)
  if skia_arch_type == 'arm':
    # Always use thumb and version 7 for arm
    gyp_defines += 'arm_thumb=1 arm_version=7 '
    if have_neon:
      gyp_defines += 'arm_neon=1 '
    else:
      gyp_defines += 'arm_neon=0 '

  os.environ['GYP_DEFINES'] = gyp_defines

  args = []
  args.extend(['--depth', '.'])
  full_path = os.path.join(target_dir, target_file)
  args.extend([full_path])
  # Common conditions
  args.extend(['-I', os.path.join(target_dir, 'common.gypi')])
  # Use the debugging format. We'll use these to create one master make file.
  args.extend(['-f', 'gypd'])

  # Off we go...
  ret = gyp.main(args)

  if ret != 0:
    raise Exception("gyp failed!")

  # Running gyp should have created a gypd file, with the same name as
  # full_path but with a 'd' on the end.
  gypd_file = full_path + 'd'
  if not os.path.exists(gypd_file):
    raise Exception("gyp failed to produce gypd file!")

  return gypd_file


def clean_gypd_files(folder):
  """Remove the gypd files generated by main().

  Args:
    folder: Folder in which to delete all files ending with 'gypd'.
  """
  assert os.path.isdir(folder)
  files = os.listdir(folder)
  for f in files:
    if f.endswith('gypd'):
      os.remove(os.path.join(folder, f))
