# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""GYP backend that generates Eclipse CDT settings files.

This backend DOES NOT generate Eclipse CDT projects. Instead, it generates XML
files that can be imported into an Eclipse CDT project. The XML file contains a
list of include paths and symbols (i.e. defines).

Because a full .cproject definition is not created by this generator, it's not
possible to properly define the include dirs and symbols for each file
individually.  Instead, one set of includes/symbols is generated for the entire
project.  This works fairly well (and is a vast improvement in general), but may
still result in a few indexer issues here and there.

This generator has no automated tests, so expect it to be broken.
"""

from xml.sax.saxutils import escape
import os.path
import subprocess
import gyp
import gyp.common
import gyp.msvs_emulation
import shlex

generator_wants_static_library_dependencies_adjusted = False

generator_default_variables = {
}

for dirname in ['INTERMEDIATE_DIR', 'PRODUCT_DIR', 'LIB_DIR', 'SHARED_LIB_DIR']:
  # Some gyp steps fail if these are empty(!).
  generator_default_variables[dirname] = 'dir'

for unused in ['RULE_INPUT_PATH', 'RULE_INPUT_ROOT', 'RULE_INPUT_NAME',
               'RULE_INPUT_DIRNAME', 'RULE_INPUT_EXT',
               'EXECUTABLE_PREFIX', 'EXECUTABLE_SUFFIX',
               'STATIC_LIB_PREFIX', 'STATIC_LIB_SUFFIX',
               'SHARED_LIB_PREFIX', 'SHARED_LIB_SUFFIX',
               'CONFIGURATION_NAME']:
  generator_default_variables[unused] = ''

# Include dirs will occasionally use the SHARED_INTERMEDIATE_DIR variable as
# part of the path when dealing with generated headers.  This value will be
# replaced dynamically for each configuration.
generator_default_variables['SHARED_INTERMEDIATE_DIR'] = \
    '$SHARED_INTERMEDIATE_DIR'


def CalculateVariables(default_variables, params):
  generator_flags = params.get('generator_flags', {})
  for key, val in generator_flags.items():
    default_variables.setdefault(key, val)
  flavor = gyp.common.GetFlavor(params)
  default_variables.setdefault('OS', flavor)
  if flavor == 'win':
    # Copy additional generator configuration data from VS, which is shared
    # by the Eclipse generator.
    import gyp.generator.msvs as msvs_generator
    generator_additional_non_configuration_keys = getattr(msvs_generator,
        'generator_additional_non_configuration_keys', [])
    generator_additional_path_sections = getattr(msvs_generator,
        'generator_additional_path_sections', [])

    gyp.msvs_emulation.CalculateCommonVariables(default_variables, params)


def CalculateGeneratorInputInfo(params):
  """Calculate the generator specific info that gets fed to input (called by
  gyp)."""
  generator_flags = params.get('generator_flags', {})
  if generator_flags.get('adjust_static_libraries', False):
    global generator_wants_static_library_dependencies_adjusted
    generator_wants_static_library_dependencies_adjusted = True


def GetAllIncludeDirectories(target_list, target_dicts,
                             shared_intermediate_dirs, config_name, params,
                             compiler_path):
  """Calculate the set of include directories to be used.

  Returns:
    A list including all the include_dir's specified for every target followed
    by any include directories that were added as cflag compiler options.
  """

  gyp_includes_set = set()
  compiler_includes_list = []

  # Find compiler's default include dirs.
  if compiler_path:
    command = shlex.split(compiler_path)
    command.extend(['-E', '-xc++', '-v', '-'])
    proc = subprocess.Popen(args=command, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output = proc.communicate()[1]
    # Extract the list of include dirs from the output, which has this format:
    #   ...
    #   #include "..." search starts here:
    #   #include <...> search starts here:
    #    /usr/include/c++/4.6
    #    /usr/local/include
    #   End of search list.
    #   ...
    in_include_list = False
    for line in output.splitlines():
      if line.startswith('#include'):
        in_include_list = True
        continue
      if line.startswith('End of search list.'):
        break
      if in_include_list:
        include_dir = line.strip()
        if include_dir not in compiler_includes_list:
          compiler_includes_list.append(include_dir)

  flavor = gyp.common.GetFlavor(params)
  if flavor == 'win':
    generator_flags = params.get('generator_flags', {})
  for target_name in target_list:
    target = target_dicts[target_name]
    if config_name in target['configurations']:
      config = target['configurations'][config_name]

      # Look for any include dirs that were explicitly added via cflags. This
      # may be done in gyp files to force certain includes to come at the end.
      # TODO(jgreenwald): Change the gyp files to not abuse cflags for this, and
      # remove this.
      if flavor == 'win':
        msvs_settings = gyp.msvs_emulation.MsvsSettings(target, generator_flags)
        cflags = msvs_settings.GetCflags(config_name)
      else:
        cflags = config['cflags']
      for cflag in cflags:
        if cflag.startswith('-I'):
          include_dir = cflag[2:]
          if include_dir not in compiler_includes_list:
            compiler_includes_list.append(include_dir)

      # Find standard gyp include dirs.
      if config.has_key('include_dirs'):
        include_dirs = config['include_dirs']
        for shared_intermediate_dir in shared_intermediate_dirs:
          for include_dir in include_dirs:
            include_dir = include_dir.replace('$SHARED_INTERMEDIATE_DIR',
                                              shared_intermediate_dir)
            if not os.path.isabs(include_dir):
              base_dir = os.path.dirname(target_name)

              include_dir = base_dir + '/' + include_dir
              include_dir = os.path.abspath(include_dir)

            gyp_includes_set.add(include_dir)

  # Generate a list that has all the include dirs.
  all_includes_list = list(gyp_includes_set)
  all_includes_list.sort()
  for compiler_include in compiler_includes_list:
    if not compiler_include in gyp_includes_set:
      all_includes_list.append(compiler_include)

  # All done.
  return all_includes_list


def GetCompilerPath(target_list, data, options):
  """Determine a command that can be used to invoke the compiler.

  Returns:
    If this is a gyp project that has explicit make settings, try to determine
    the compiler from that.  Otherwise, see if a compiler was specified via the
    CC_target environment variable.
  """
  # First, see if the compiler is configured in make's settings.
  build_file, _, _ = gyp.common.ParseQualifiedTarget(target_list[0])
  make_global_settings_dict = data[build_file].get('make_global_settings', {})
  for key, value in make_global_settings_dict:
    if key in ['CC', 'CXX']:
      return os.path.join(options.toplevel_dir, value)

  # Check to see if the compiler was specified as an environment variable.
  for key in ['CC_target', 'CC', 'CXX']:
    compiler = os.environ.get(key)
    if compiler:
      return compiler

  return 'gcc'


def GetAllDefines(target_list, target_dicts, data, config_name, params,
                  compiler_path):
  """Calculate the defines for a project.

  Returns:
    A dict that includes explict defines declared in gyp files along with all of
    the default defines that the compiler uses.
  """

  # Get defines declared in the gyp files.
  all_defines = {}
  flavor = gyp.common.GetFlavor(params)
  if flavor == 'win':
    generator_flags = params.get('generator_flags', {})
  for target_name in target_list:
    target = target_dicts[target_name]

    if flavor == 'win':
      msvs_settings = gyp.msvs_emulation.MsvsSettings(target, generator_flags)
      extra_defines = msvs_settings.GetComputedDefines(config_name)
    else:
      extra_defines = []
    if config_name in target['configurations']:
      config = target['configurations'][config_name]
      target_defines = config['defines']
    else:
      target_defines = []
    for define in target_defines + extra_defines:
      split_define = define.split('=', 1)
      if len(split_define) == 1:
        split_define.append('1')
      if split_define[0].strip() in all_defines:
        # Already defined
        continue
      all_defines[split_define[0].strip()] = split_define[1].strip()
  # Get default compiler defines (if possible).
  if flavor == 'win':
    return all_defines  # Default defines already processed in the loop above.
  if compiler_path:
    command = shlex.split(compiler_path)
    command.extend(['-E', '-dM', '-'])
    cpp_proc = subprocess.Popen(args=command, cwd='.',
                                stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    cpp_output = cpp_proc.communicate()[0]
    cpp_lines = cpp_output.split('\n')
    for cpp_line in cpp_lines:
      if not cpp_line.strip():
        continue
      cpp_line_parts = cpp_line.split(' ', 2)
      key = cpp_line_parts[1]
      if len(cpp_line_parts) >= 3:
        val = cpp_line_parts[2]
      else:
        val = '1'
      all_defines[key] = val

  return all_defines


def WriteIncludePaths(out, eclipse_langs, include_dirs):
  """Write the includes section of a CDT settings export file."""

  out.write('  <section name="org.eclipse.cdt.internal.ui.wizards.' \
            'settingswizards.IncludePaths">\n')
  out.write('    <language name="holder for library settings"></language>\n')
  for lang in eclipse_langs:
    out.write('    <language name="%s">\n' % lang)
    for include_dir in include_dirs:
      out.write('      <includepath workspace_path="false">%s</includepath>\n' %
                include_dir)
    out.write('    </language>\n')
  out.write('  </section>\n')


def WriteMacros(out, eclipse_langs, defines):
  """Write the macros section of a CDT settings export file."""

  out.write('  <section name="org.eclipse.cdt.internal.ui.wizards.' \
            'settingswizards.Macros">\n')
  out.write('    <language name="holder for library settings"></language>\n')
  for lang in eclipse_langs:
    out.write('    <language name="%s">\n' % lang)
    for key in sorted(defines.iterkeys()):
      out.write('      <macro><name>%s</name><value>%s</value></macro>\n' %
                (escape(key), escape(defines[key])))
    out.write('    </language>\n')
  out.write('  </section>\n')


def GenerateOutputForConfig(target_list, target_dicts, data, params,
                            config_name):
  options = params['options']
  generator_flags = params.get('generator_flags', {})

  # build_dir: relative path from source root to our output files.
  # e.g. "out/Debug"
  build_dir = os.path.join(generator_flags.get('output_dir', 'out'),
                           config_name)

  toplevel_build = os.path.join(options.toplevel_dir, build_dir)
  # Ninja uses out/Debug/gen while make uses out/Debug/obj/gen as the
  # SHARED_INTERMEDIATE_DIR. Include both possible locations.
  shared_intermediate_dirs = [os.path.join(toplevel_build, 'obj', 'gen'),
                              os.path.join(toplevel_build, 'gen')]

  out_name = os.path.join(toplevel_build, 'eclipse-cdt-settings.xml')
  gyp.common.EnsureDirExists(out_name)
  out = open(out_name, 'w')

  out.write('<?xml version="1.0" encoding="UTF-8"?>\n')
  out.write('<cdtprojectproperties>\n')

  eclipse_langs = ['C++ Source File', 'C Source File', 'Assembly Source File',
                   'GNU C++', 'GNU C', 'Assembly']
  compiler_path = GetCompilerPath(target_list, data, options)
  include_dirs = GetAllIncludeDirectories(target_list, target_dicts,
                                          shared_intermediate_dirs, config_name,
                                          params, compiler_path)
  WriteIncludePaths(out, eclipse_langs, include_dirs)
  defines = GetAllDefines(target_list, target_dicts, data, config_name, params,
                          compiler_path)
  WriteMacros(out, eclipse_langs, defines)

  out.write('</cdtprojectproperties>\n')
  out.close()


def GenerateOutput(target_list, target_dicts, data, params):
  """Generate an XML settings file that can be imported into a CDT project."""

  if params['options'].generator_output:
    raise NotImplementedError, "--generator_output not implemented for eclipse"

  user_config = params.get('generator_flags', {}).get('config', None)
  if user_config:
    GenerateOutputForConfig(target_list, target_dicts, data, params,
                            user_config)
  else:
    config_names = target_dicts[target_list[0]]['configurations'].keys()
    for config_name in config_names:
      GenerateOutputForConfig(target_list, target_dicts, data, params,
                              config_name)

