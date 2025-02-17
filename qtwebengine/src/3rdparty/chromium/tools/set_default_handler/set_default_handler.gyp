# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'set_default_handler',
      'type': 'executable',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../ui/base/ui_base.gyp:ui_base',
        '../../ui/gfx/gfx.gyp:gfx',
        '../../ui/gfx/gfx.gyp:gfx_geometry',
        '../../win8/win8.gyp:test_support_win8',
      ],
      'include_dirs': [
        '../..',
      ],
      'sources': [
        'set_default_handler_main.cc',
      ],
    },
  ],
}
