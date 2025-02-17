# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file is meant to be included into a target to provide a rule
# to "build" .isolate files into a .isolated file.
#
# To use this, create a gyp target with the following form:
# 'conditions': [
#   ['test_isolation_mode != "noop"', {
#     'targets': [
#       {
#         'target_name': 'foo_test_run',
#         'type': 'none',
#         'dependencies': [
#           'foo_test',
#         ],
#         'includes': [
#           '../build/isolate.gypi',
#           'foo_test.isolate',
#         ],
#         'sources': [
#           'foo_test.isolate',
#         ],
#       },
#     ],
#   }],
# ],
#
# Note: foo_test.isolate is included and a source file. It is an inherent
# property of the .isolate format. This permits to define GYP variables but is
# a stricter format than GYP so isolate.py can read it.
#
# The generated .isolated file will be:
#   <(PRODUCT_DIR)/foo_test.isolated
#
# See http://dev.chromium.org/developers/testing/isolated-testing/for-swes
# for more information.

{
  'includes': [
    '../build/util/version.gypi',
  ],
  'rules': [
    {
      'rule_name': 'isolate',
      'extension': 'isolate',
      'inputs': [
        # Files that are known to be involved in this step.
        '<(DEPTH)/tools/isolate_driver.py',
        '<(DEPTH)/tools/swarming_client/isolate.py',
        '<(DEPTH)/tools/swarming_client/run_isolated.py',

        # Disable file tracking by the build driver for now. This means the
        # project must have the proper build-time dependency for their runtime
        # dependency. This improves the runtime of the build driver since it
        # doesn't have to stat() all these files.
        #
        # More importantly, it means that even if a isolate_dependency_tracked
        # file is missing, for example if a file was deleted and the .isolate
        # file was not updated, that won't break the build, especially in the
        # case where foo_tests_run is not built! This should be reenabled once
        # the switch-over to running tests on Swarm is completed.
        #'<@(isolate_dependency_tracked)',
      ],
      'outputs': [
        '<(PRODUCT_DIR)/<(RULE_INPUT_ROOT).isolated',
      ],
      'action': [
        'python',
        '<(DEPTH)/tools/isolate_driver.py',
        '<(test_isolation_mode)',
        '--isolated', '<@(_outputs)',
        '--isolate', '<(RULE_INPUT_PATH)',

        # Variables should use the -V FOO=<(FOO) form so frequent values,
        # like '0' or '1', aren't stripped out by GYP. Run 'isolate.py help' for
        # more details.
        #
        # This list needs to be kept in sync with the cmd line options
        # in src/build/android/pylib/gtest/setup.py.

        # Path variables are used to replace file paths when loading a .isolate
        # file
        '--path-variable', 'DEPTH', '<(DEPTH)',
        '--path-variable', 'PRODUCT_DIR', '<(PRODUCT_DIR) ',

        # Extra variables are replaced on the 'command' entry and on paths in
        # the .isolate file but are not considered relative paths.
        '--extra-variable', 'version_full=<(version_full)',

        '--config-variable', 'OS=<(OS)',
        '--config-variable', 'chromeos=<(chromeos)',
        '--config-variable', 'component=<(component)',
        # TODO(kbr): move this to chrome_tests.gypi:gles2_conform_tests_run
        # once support for user-defined config variables is added.
        '--config-variable',
          'internal_gles2_conform_tests=<(internal_gles2_conform_tests)',
        '--config-variable', 'icu_use_data_file_flag=<(icu_use_data_file_flag)',
        '--config-variable', 'libpeer_target_type=<(libpeer_target_type)',
        '--config-variable', 'use_openssl=<(use_openssl)',
      ],
      'conditions': [
        # Note: When gyp merges lists, it appends them to the old value.
        ['OS=="mac"', {
          # <(mac_product_name) can contain a space, so don't use FOO=<(FOO)
          # form.
          'action': [
            '--extra-variable', 'mac_product_name', '<(mac_product_name)',
          ],
        }],
        ["test_isolation_outdir!=''", {
          'action': [ '--isolate-server', '<(test_isolation_outdir)' ],
        }],
        ['test_isolation_fail_on_missing == 0', {
          'action': ['--ignore_broken_items'],
        }],
      ],
    },
  ],
}
