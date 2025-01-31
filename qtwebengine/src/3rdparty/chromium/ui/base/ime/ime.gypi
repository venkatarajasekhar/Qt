# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'dependencies' : [
    '<(DEPTH)/ui/events/events.gyp:events',
  ],
  'sources': [
    'candidate_window.cc',
    'candidate_window.h',
    'chromeos/character_composer.cc',
    'chromeos/character_composer.h',
    'chromeos/ime_bridge.cc',
    'chromeos/ime_bridge.h',
    'chromeos/ime_keymap.cc',
    'chromeos/ime_keymap.h',
    'chromeos/mock_ime_candidate_window_handler.cc',
    'chromeos/mock_ime_candidate_window_handler.h',
    'chromeos/mock_ime_engine_handler.cc',
    'chromeos/mock_ime_engine_handler.h',
    'chromeos/mock_ime_input_context_handler.cc',
    'chromeos/mock_ime_input_context_handler.h',
    'composition_text.cc',
    'composition_text.h',
    'composition_text_util_pango.cc',
    'composition_text_util_pango.h',
    'composition_underline.h',
    'dummy_input_method_delegate.cc',
    'dummy_input_method_delegate.h',
    'infolist_entry.cc',
    'infolist_entry.h',
    'input_method.h',
    'input_method_auralinux.cc',
    'input_method_auralinux.h',
    'input_method_base.cc',
    'input_method_base.h',
    'input_method_chromeos.cc',
    'input_method_chromeos.h',
    'input_method_delegate.h',
    'input_method_factory.cc',
    'input_method_factory.h',
    'input_method_initializer.cc',
    'input_method_initializer.h',
    'input_method_mac.h',
    'input_method_mac.mm',
    'input_method_minimal.cc',
    'input_method_minimal.h',
    'input_method_observer.h',
    'input_method_win.cc',
    'input_method_win.h',
    'linux/fake_input_method_context.cc',
    'linux/fake_input_method_context.h',
    'linux/fake_input_method_context_factory.cc',
    'linux/fake_input_method_context_factory.h',
    'linux/linux_input_method_context.h',
    'linux/linux_input_method_context_factory.cc',
    'linux/linux_input_method_context_factory.h',
    'mock_input_method.cc',
    'mock_input_method.h',
    'remote_input_method_delegate_win.h',
    'remote_input_method_win.cc',
    'remote_input_method_win.h',
    'text_input_client.cc',
    'text_input_client.h',
    'text_input_focus_manager.cc',
    'text_input_focus_manager.h',
    'text_input_type.h',
    'win/imm32_manager.cc',
    'win/imm32_manager.h',
    'win/tsf_input_scope.cc',
    'win/tsf_input_scope.h',
  ],
  'conditions': [
    ['toolkit_views==0 and use_aura==0', {
      'sources!': [
        'input_method_factory.cc',
        'input_method_factory.h',
        'input_method_minimal.cc',
        'input_method_minimal.h',
      ],
    }],
    ['chromeos==0 or use_x11==0', {
      'sources!': [
        'input_method_chromeos.cc',
        'input_method_chromeos.h',
      ],
    }],
    ['chromeos==1', {
      'dependencies': [
        '<(DEPTH)/chromeos/chromeos.gyp:chromeos',
      ],
    }],
    ['OS!="win"', {
      'sources!': [
        'input_method_imm32.cc',
        'input_method_imm32.h',
      ],
    }],
    ['use_aura==0 or (desktop_linux==0 and use_ozone==0)', {
      'sources!': [
        'input_method_auralinux.cc',
        'input_method_auralinux.h',
        'linux/fake_input_method_context.cc',
        'linux/fake_input_method_context.h',
        'linux/fake_input_method_context_factory.cc',
        'linux/fake_input_method_context_factory.h',
        'linux/linux_input_method_context.h',
        'linux/linux_input_method_context_factory.cc',
        'linux/linux_input_method_context_factory.h',
      ],
    }],
    ['use_x11==0', {
      'sources!': [
        'composition_text_util_pango.cc',
        'composition_text_util_pango.h',
        'chromeos/character_composer.cc',
        'chromeos/character_composer.h',
      ],
    }],
    ['OS=="android"', {
      'dependencies!' : [
        '<(DEPTH)/ui/events/events.gyp:events',
      ],
    }]
  ],
}
