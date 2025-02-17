#!/usr/bin/python

# Copyright 2014 Google Inc.
#
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Code for generating Android.mk for a tool."""


import android_framework_gyp
import gypd_parser
import makefile_writer
import os
import vars_dict_lib


def write_tool_android_mk(target_dir, var_dict, place_in_local_tmp):
  """Write Android.mk for a Skia tool.

  Args:
    target_dir: Destination for the makefile. Must not be None.
    var_dict: VarsDict containing variables for the makefile.
    place_in_local_tmp: If True, the executable will be synced to
      /data/local/tmp.
  """
  target_file = os.path.join(target_dir, 'Android.mk')
  with open(target_file, 'w') as f:
    f.write(makefile_writer.AUTOGEN_WARNING)

    if place_in_local_tmp:
      f.write('local_target_dir := $(TARGET_OUT_DATA)/local/tmp\n')

    makefile_writer.write_local_path(f)
    makefile_writer.write_clear_vars(f)

    makefile_writer.write_local_vars(f, var_dict, False, None)

    if place_in_local_tmp:
      f.write('LOCAL_MODULE_PATH := $(local_target_dir)\n')

    makefile_writer.write_include_stlport(f)
    f.write('include $(BUILD_EXECUTABLE)\n')


def generate_tool(gyp_dir, target_file, skia_trunk, dest_dir,
                  skia_lib_var_dict, local_module_name, local_module_tags,
                  place_in_local_tmp=False):
  """Common steps for building one of the skia tools.

  Parse a gyp file and create an Android.mk for this tool.

  Args:
    gyp_dir: Directory containing gyp files.
    target_file: gyp file for the project to be built, contained in gyp_dir.
    skia_trunk: Trunk of Skia, used for determining the destination to write
      'Android.mk'.
    dest_dir: Destination for 'Android.mk', relative to skia_trunk. Used for
      both writing relative paths in the makefile and for determining the
      destination to write the it.
    skia_lib_var_dict: VarsDict representing libskia. Used as a reference to
      ensure we do not duplicate anything in this Android.mk.
    local_module_name: Name for this tool, to set as LOCAL_MODULE.
    local_module_tags: Tags to pass to LOCAL_MODULE_TAG.
    place_in_local_tmp: If True, the executable will be synced to
      /data/local/tmp.
  """
  result_file = android_framework_gyp.main(target_dir=gyp_dir,
                                           target_file=target_file,
                                           skia_arch_type='other',
                                           have_neon=False)

  var_dict = vars_dict_lib.VarsDict()

  # Add known targets from skia_lib, so we do not reparse them.
  var_dict.KNOWN_TARGETS.set(skia_lib_var_dict.KNOWN_TARGETS)

  gypd_parser.parse_gypd(var_dict, result_file, dest_dir)

  android_framework_gyp.clean_gypd_files(gyp_dir)

  var_dict.LOCAL_MODULE.add(local_module_name)
  for tag in local_module_tags:
    var_dict.LOCAL_MODULE_TAGS.add(tag)

  # No need for defines that are already in skia_lib.
  for define in skia_lib_var_dict.DEFINES:
    try:
      var_dict.DEFINES.remove(define)
    except ValueError:
      # Okay if the define was not part of the parse for our tool.
      pass

  if skia_trunk:
    full_dest = os.path.join(skia_trunk, dest_dir)
  else:
    full_dest = dest_dir

  # If the path does not exist, create it. This will happen during testing,
  # where there is no subdirectory for each tool (just a temporary folder).
  if not os.path.exists(full_dest):
    os.mkdir(full_dest)

  write_tool_android_mk(target_dir=full_dest, var_dict=var_dict,
                        place_in_local_tmp=place_in_local_tmp)
