# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'cpu_features',
      'type': 'static_library',
      'direct_dependent_settings': {
        'include_dirs': [
          'sources/android/cpufeatures',
        ],
      },
      'sources': [
        'sources/android/cpufeatures/cpu-features.c',
      ],
    },
  ],
}
