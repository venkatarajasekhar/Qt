# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generate IDL bindings for modules, plus aggregate bindings files.
#
# Design doc: http://www.chromium.org/developers/design-documents/idl-build

{
  'includes': [
    # ../../.. == Source
    '../../../bindings/bindings.gypi',
    '../../../bindings/modules/idl.gypi',
    '../../../bindings/modules/modules.gypi',
    '../../../bindings/scripts/scripts.gypi',
    '../../../modules/modules.gypi',
    'generated.gypi',
  ],

  'targets': [
################################################################################
  {
    'target_name': 'bindings_modules_v8_generated_individual',
    'type': 'none',
    # The 'binding' rule generates .h files, so mark as hard_dependency, per:
    # https://code.google.com/p/gyp/wiki/InputFormatReference#Linking_Dependencies
    'hard_dependency': 1,
    'dependencies': [
      '../../core/generated.gyp:core_global_constructors_idls',
      '../generated.gyp:modules_global_constructors_idls',
      '../generated.gyp:interfaces_info',
      '<(bindings_scripts_dir)/scripts.gyp:cached_jinja_templates',
      '<(bindings_scripts_dir)/scripts.gyp:cached_lex_yacc_tables',
    ],
    'sources': [
      '<@(modules_interface_idl_files)',
    ],
    'rules': [{
      'rule_name': 'binding',
      'extension': 'idl',
      'msvs_external_rule': 1,
      'inputs': [
        '<@(idl_lexer_parser_files)',  # to be explicit (covered by parsetab)
        '<@(idl_compiler_files)',
        '<(bindings_scripts_output_dir)/lextab.py',
        '<(bindings_scripts_output_dir)/parsetab.pickle',
        '<(bindings_scripts_output_dir)/cached_jinja_templates.stamp',
        '<(bindings_dir)/IDLExtendedAttributes.txt',
        # If the dependency structure or public interface info (e.g.,
        # [ImplementedAs]) changes, we rebuild all files, since we're not
        # computing dependencies file-by-file in the build.
        # This data is generally stable.
        '<(bindings_modules_output_dir)/InterfacesInfoModules.pickle',
        # Further, if any dependency (partial interface or implemented
        # interface) changes, rebuild everything, since every IDL potentially
        # depends on them, because we're not computing dependencies
        # file-by-file.
        # FIXME: This is too conservative, and causes excess rebuilds:
        # compute this file-by-file.  http://crbug.com/341748
        '<@(all_dependency_idl_files)',
      ],
      'outputs': [
        '<(bindings_modules_v8_output_dir)/V8<(RULE_INPUT_ROOT).cpp',
        '<(bindings_modules_v8_output_dir)/V8<(RULE_INPUT_ROOT).h',
      ],
      # sanitize-win-build-log.sed uses a regex which matches this command
      # line (Python script + .idl file being processed).
      # Update that regex if command line changes (other than changing flags)
      'action': [
        'python',
        '<(bindings_scripts_dir)/idl_compiler.py',
        '--cache-dir',
        '<(bindings_scripts_output_dir)',
        '--output-dir',
        '<(bindings_modules_v8_output_dir)',
        '--interfaces-info',
        '<(bindings_modules_output_dir)/InterfacesInfoModules.pickle',
        '--write-file-only-if-changed',
        '<(write_file_only_if_changed)',
        '<(RULE_INPUT_PATH)',
      ],
      'message': 'Generating binding from <(RULE_INPUT_PATH)',
    }],
  },
################################################################################
  {
    'target_name': 'bindings_modules_v8_generated_aggregate',
    'type': 'none',
    'actions': [{
      'action_name': 'generate_aggregate_bindings_modules_v8',
      'inputs': [
        '<(bindings_scripts_dir)/aggregate_generated_bindings.py',
        '<(modules_idl_files_list)',
      ],
      'outputs': [
        '<@(bindings_modules_v8_generated_aggregate_files)',
      ],
      'action': [
        'python',
        '<(bindings_scripts_dir)/aggregate_generated_bindings.py',
        'modules',
        '<(modules_idl_files_list)',
        '--',
        '<@(bindings_modules_v8_generated_aggregate_files)',
      ],
      'message': 'Generating aggregate generated modules V8 bindings files',
    }],
  },
################################################################################
  {
    'target_name': 'bindings_modules_v8_generated',
    'type': 'none',
    'dependencies': [
      'bindings_modules_v8_generated_aggregate',
      'bindings_modules_v8_generated_individual',
    ],
  },
################################################################################
  ],  # targets
}
