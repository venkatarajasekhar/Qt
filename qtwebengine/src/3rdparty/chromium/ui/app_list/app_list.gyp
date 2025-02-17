# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'app_list',
      'type': '<(component)',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:base_i18n',
        '../../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../../skia/skia.gyp:skia',
        '../base/ui_base.gyp:ui_base',
        '../compositor/compositor.gyp:compositor',
        '../events/events.gyp:events_base',
        '../gfx/gfx.gyp:gfx',
        '../gfx/gfx.gyp:gfx_geometry',
        '../resources/ui_resources.gyp:ui_resources',
        '../strings/ui_strings.gyp:ui_strings',
      ],
      'defines': [
        'APP_LIST_IMPLEMENTATION',
      ],
      'sources': [
        'app_list_constants.cc',
        'app_list_constants.h',
        'app_list_export.h',
        'app_list_folder_item.cc',
        'app_list_folder_item.h',
        'app_list_item.cc',
        'app_list_item.h',
        'app_list_item_observer.h',
        'app_list_item_list.cc',
        'app_list_item_list.h',
        'app_list_item_list_observer.h',
        'app_list_menu.cc',
        'app_list_menu.h',
        'app_list_model.cc',
        'app_list_model.h',
        'app_list_model_observer.h',
        'app_list_switches.cc',
        'app_list_switches.h',
        'app_list_view_delegate.cc',
        'app_list_view_delegate.h',
        'cocoa/app_list_pager_view.h',
        'cocoa/app_list_pager_view.mm',
        'cocoa/app_list_view_controller.h',
        'cocoa/app_list_view_controller.mm',
        'cocoa/app_list_window_controller.h',
        'cocoa/app_list_window_controller.mm',
        'cocoa/apps_collection_view_drag_manager.h',
        'cocoa/apps_collection_view_drag_manager.mm',
        'cocoa/apps_grid_controller.h',
        'cocoa/apps_grid_controller.mm',
        'cocoa/apps_grid_view_item.h',
        'cocoa/apps_grid_view_item.mm',
        'cocoa/apps_pagination_model_observer.h',
        'cocoa/apps_search_box_controller.h',
        'cocoa/apps_search_box_controller.mm',
        'cocoa/apps_search_results_controller.h',
        'cocoa/apps_search_results_controller.mm',
        'cocoa/apps_search_results_model_bridge.h',
        'cocoa/apps_search_results_model_bridge.mm',
        'cocoa/item_drag_controller.h',
        'cocoa/item_drag_controller.mm',
        'cocoa/scroll_view_with_no_scrollbars.h',
        'cocoa/scroll_view_with_no_scrollbars.mm',
        'pagination_model.cc',
        'pagination_model.h',
        'pagination_model_observer.h',
        'search_box_model.cc',
        'search_box_model.h',
        'search_box_model_observer.h',
        'search_provider.cc',
        'search_provider.h',
        'search_result.cc',
        'search_result.h',
        'speech_ui_model.cc',
        'speech_ui_model.h',
        'speech_ui_model_observer.h',
        'views/apps_container_view.cc',
        'views/apps_container_view.h',
        'views/app_list_background.cc',
        'views/app_list_background.h',
        'views/app_list_drag_and_drop_host.h',
        'views/app_list_folder_view.cc',
        'views/app_list_folder_view.h',
        'views/app_list_item_view.cc',
        'views/app_list_item_view.h',
        'views/app_list_main_view.cc',
        'views/app_list_main_view.h',
        'views/app_list_menu_views.cc',
        'views/app_list_menu_views.h',
        'views/app_list_view.cc',
        'views/app_list_view.h',
        'views/app_list_view_observer.h',
        'views/apps_grid_view.cc',
        'views/apps_grid_view.h',
        'views/apps_grid_view_delegate.h',
        'views/apps_grid_view_folder_delegate.h',
        'views/cached_label.cc',
        'views/cached_label.h',
        'views/contents_switcher_view.cc',
        'views/contents_switcher_view.h',
        'views/contents_view.cc',
        'views/contents_view.h',
        'views/folder_background_view.cc',
        'views/folder_background_view.h',
        'views/folder_header_view.cc',
        'views/folder_header_view.h',
        'views/folder_header_view_delegate.h',
        'views/page_switcher.cc',
        'views/page_switcher.h',
        'views/progress_bar_view.cc',
        'views/progress_bar_view.h',
        'views/pulsing_block_view.cc',
        'views/pulsing_block_view.h',
        'views/search_box_view.cc',
        'views/search_box_view.h',
        'views/search_box_view_delegate.h',
        'views/search_result_actions_view.cc',
        'views/search_result_actions_view.h',
        'views/search_result_list_view.cc',
        'views/search_result_list_view.h',
        'views/search_result_list_view_delegate.h',
        'views/search_result_view.cc',
        'views/search_result_view.h',
        'views/speech_view.cc',
        'views/speech_view.h',
        'views/start_page_view.cc',
        'views/start_page_view.h',
        'views/tile_item_view.cc',
        'views/tile_item_view.h',
        'views/top_icon_animation_view.cc',
        'views/top_icon_animation_view.h',
      ],
      'conditions': [
        ['use_aura==1', {
          'dependencies': [
            '../aura/aura.gyp:aura',
            '../wm/wm.gyp:wm',
          ],
        }],
        ['toolkit_views==1', {
          'dependencies': [
            '../events/events.gyp:events',
            '../views/views.gyp:views',
          ],
        }, {  # toolkit_views==0
          'sources/': [
            ['exclude', 'views/'],
          ],
        }],
        ['OS=="mac"', {
          'dependencies': [
            '../../third_party/google_toolbox_for_mac/google_toolbox_for_mac.gyp:google_toolbox_for_mac',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
            ],
          },
        }, {  # OS!="mac"
          'sources/': [
            ['exclude', 'cocoa/'],
          ],
        }],
      ],
      # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
    {
      'target_name': 'app_list_test_support',
      'type': 'static_library',
      'dependencies': [
        '../../base/base.gyp:base',
        '../gfx/gfx.gyp:gfx',
        '../gfx/gfx.gyp:gfx_geometry',
        '../resources/ui_resources.gyp:ui_resources',
        'app_list',
      ],
      'sources': [
        'test/app_list_test_model.cc',
        'test/app_list_test_model.h',
        'test/app_list_test_view_delegate.cc',
        'test/app_list_test_view_delegate.h',
      ],
    },
    {
      'target_name': 'app_list_unittests',
      'type': 'executable',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:test_support_base',
        '../../skia/skia.gyp:skia',
        '../../testing/gtest.gyp:gtest',
        '../base/ui_base.gyp:ui_base',
        '../compositor/compositor.gyp:compositor',
        '../resources/ui_resources.gyp:ui_resources',
        '../resources/ui_resources.gyp:ui_test_pak',
        'app_list',
        'app_list_test_support',
      ],
      'sources': [
        'app_list_item_list_unittest.cc',
        'app_list_model_unittest.cc',
        'pagination_model_unittest.cc',
        'cocoa/app_list_view_controller_unittest.mm',
        'cocoa/app_list_window_controller_unittest.mm',
        'cocoa/apps_grid_controller_unittest.mm',
        'cocoa/apps_search_box_controller_unittest.mm',
        'cocoa/apps_search_results_controller_unittest.mm',
        'cocoa/test/apps_grid_controller_test_helper.h',
        'cocoa/test/apps_grid_controller_test_helper.mm',
        'test/run_all_unittests.cc',
        'views/app_list_main_view_unittest.cc',
        'views/app_list_view_unittest.cc',
        'views/apps_grid_view_unittest.cc',
        'views/folder_header_view_unittest.cc',
        'views/search_box_view_unittest.cc',
        'views/search_result_list_view_unittest.cc',
        'views/speech_view_unittest.cc',
        'views/test/apps_grid_view_test_api.cc',
        'views/test/apps_grid_view_test_api.h',
      ],
      'conditions': [
        ['toolkit_views==1', {
          'dependencies': [
            '../views/views.gyp:views',
            '../views/views.gyp:views_test_support',
          ],
        }, {  # toolkit_views==0
          'sources/': [
            ['exclude', 'views/'],
          ]
        }],
        ['OS=="mac"', {
          'dependencies': [
            '../events/events.gyp:events_test_support',
            '../gfx/gfx.gyp:gfx_test_support',
          ],
          'conditions': [
            ['component=="static_library"', {
              # Needed to link to Obj-C static libraries.
              'xcode_settings': {'OTHER_LDFLAGS': ['-Wl,-ObjC']},
            }],
          ],
        }, {  # OS!="mac"
          'sources/': [
            ['exclude', 'cocoa/'],
          ],
        }],
        # See http://crbug.com/162998#c4 for why this is needed.
        ['OS=="linux" and use_allocator!="none"', {
          'dependencies': [
            '../../base/allocator/allocator.gyp:allocator',
            # The following two dependencies provide the missing
            # symbol HeapProfilerStart in Linux component builds.
            # They probably can be removed after http://crbug.com/263316
            '../../webkit/child/webkit_child.gyp:webkit_child',
          ],
        }],
        ['OS=="win" and win_use_allocator_shim==1', {
          'dependencies': [
            '../../base/allocator/allocator.gyp:allocator',
          ],
        }],
      ],
      # Disable c4267 warnings until we fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
  ],
  'conditions': [
    ['toolkit_views==1', {
      'targets': [
        {
          'target_name': 'app_list_demo',
          'type': 'executable',
          'sources': [
            '../../content/app/startup_helper_win.cc',
            'demo/app_list_demo_views.cc',
          ],
          'dependencies': [
            '../../base/base.gyp:base',
            '../../content/content.gyp:content',
            '../../content/content.gyp:content_browser',
            '../../skia/skia.gyp:skia',
            '../../url/url.gyp:url_lib',
            '../base/ui_base.gyp:ui_base',
            '../events/events.gyp:events',
            '../resources/ui_resources.gyp:ui_resources',
            '../resources/ui_resources.gyp:ui_test_pak',
            '../views/controls/webview/webview.gyp:webview',
            '../views/views.gyp:views',
            '../views_content_client/views_content_client.gyp:views_content_client',
            'app_list',
            'app_list_test_support',
          ],
          'conditions': [
            ['OS=="win"', {
              'msvs_settings': {
                'VCLinkerTool': {
                  'SubSystem': '2',  # Set /SUBSYSTEM:WINDOWS
                },
              },
              'dependencies': [
                '../../sandbox/sandbox.gyp:sandbox',
              ],
            }],
          ],
        },
      ],
    }],  # toolkit_views==1
  ],
}
