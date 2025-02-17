# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'dependencies': [
    '../base/base.gyp:base',
    '../components/tracing.gyp:tracing',
    '../gpu/command_buffer/command_buffer.gyp:gles2_utils',
    '../net/net.gyp:net',
    '../skia/skia.gyp:skia',
    '../third_party/WebKit/public/blink_headers.gyp:blink_headers',
    '../third_party/icu/icu.gyp:icuuc',
    '../third_party/libjingle/libjingle.gyp:libjingle',
    '../ui/accessibility/accessibility.gyp:accessibility',
    '../ui/base/ui_base.gyp:ui_base',
    '../ui/events/ipc/events_ipc.gyp:events_ipc',
    '../ui/gfx/gfx.gyp:gfx',
    '../ui/gfx/gfx.gyp:gfx_geometry',
    '../ui/shell_dialogs/shell_dialogs.gyp:shell_dialogs',
    '../url/url.gyp:url_lib',
  ],
  'include_dirs': [
    '..',
  ],
  'export_dependent_settings': [
    '../base/base.gyp:base',
    # The public content API headers directly include Blink API headers, so we
    # have to export the blink header settings so that relative paths in these
    # headers resolve correctly.
    '../third_party/WebKit/public/blink_headers.gyp:blink_headers',
  ],
  'variables': {
    'public_common_sources': [
      'public/common/bindings_policy.h',
      'public/common/child_process_host.h',
      'public/common/child_process_host_delegate.cc',
      'public/common/child_process_host_delegate.h',
      'public/common/child_process_sandbox_support_linux.h',
      'public/common/color_suggestion.cc',
      'public/common/color_suggestion.h',
      'public/common/common_param_traits.cc',
      'public/common/common_param_traits.h',
      'public/common/common_param_traits_macros.h',
      'public/common/console_message_level.h',
      'public/common/content_client.cc',
      'public/common/content_client.h',
      'public/common/content_constants.cc',
      'public/common/content_constants.h',
      'public/common/content_descriptors.h',
      'public/common/content_ipc_logging.h',
      'public/common/content_paths.h',
      'public/common/content_switches.cc',
      'public/common/content_switches.h',
      'public/common/context_menu_params.cc',
      'public/common/context_menu_params.h',
      'public/common/drop_data.cc',
      'public/common/drop_data.h',
      'public/common/eme_codec.h',
      'public/common/favicon_url.cc',
      'public/common/favicon_url.h',
      'public/common/file_chooser_params.cc',
      'public/common/file_chooser_params.h',
      'public/common/frame_navigate_params.cc',
      'public/common/frame_navigate_params.h',
      'public/common/geoposition.cc',
      'public/common/geoposition.h',
      'public/common/gpu_memory_stats.cc',
      'public/common/gpu_memory_stats.h',
      'public/common/injection_test_mac.h',
      'public/common/injection_test_win.h',
      'public/common/javascript_message_type.h',
      'public/common/main_function_params.h',
      'public/common/media_stream_request.cc',
      'public/common/media_stream_request.h',
      'public/common/menu_item.cc',
      'public/common/menu_item.h',
      'public/common/page_state.cc',
      'public/common/page_state.h',
      'public/common/page_transition_types.cc',
      'public/common/page_transition_types.h',
      'public/common/page_transition_types_list.h',
      'public/common/page_type.h',
      'public/common/page_zoom.h',
      'public/common/pepper_plugin_info.cc',
      'public/common/pepper_plugin_info.h',
      'public/common/process_type.h',
      'public/common/referrer.h',
      'public/common/renderer_preferences.cc',
      'public/common/renderer_preferences.h',
      'public/common/resource_devtools_info.cc',
      'public/common/resource_devtools_info.h',
      'public/common/resource_response.h',
      'public/common/resource_response_info.cc',
      'public/common/resource_response_info.h',
      'public/common/result_codes.h',
      'public/common/result_codes_list.h',
      'public/common/sandbox_init.h',
      'public/common/sandbox_linux.h',
      'public/common/sandbox_type_mac.h',
      'public/common/sandboxed_process_launcher_delegate.cc',
      'public/common/sandboxed_process_launcher_delegate.h',
      'public/common/security_style.h',
      'public/common/show_desktop_notification_params.cc',
      'public/common/show_desktop_notification_params.h',
      'public/common/signed_certificate_timestamp_id_and_status.cc',
      'public/common/signed_certificate_timestamp_id_and_status.h',
      'public/common/speech_recognition_error.h',
      'public/common/speech_recognition_grammar.h',
      'public/common/speech_recognition_result.cc',
      'public/common/speech_recognition_result.h',
      'public/common/ssl_status.cc',
      'public/common/ssl_status.h',
      'public/common/stop_find_action.h',
      'public/common/storage_quota_params.h',
      'public/common/three_d_api_types.h',
      'public/common/top_controls_state.h',
      'public/common/top_controls_state_list.h',
      'public/common/url_constants.cc',
      'public/common/url_constants.h',
      'public/common/url_fetcher.h',
      'public/common/url_utils.cc',
      'public/common/url_utils.h',
      'public/common/user_agent.h',
      'public/common/webplugininfo.cc',
      'public/common/webplugininfo.h',
      'public/common/window_container_type.cc',
      'public/common/window_container_type.h',
      'public/common/zygote_fork_delegate_linux.h',
    ],
    'private_common_sources': [
      'common/accessibility_messages.h',
      'common/all_messages.h',
      'common/android/address_parser.cc',
      'common/android/address_parser.h',
      'common/android/address_parser_internal.cc',
      'common/android/address_parser_internal.h',
      'common/android/common_jni_registrar.cc',
      'common/android/common_jni_registrar.h',
      'common/android/gin_java_bridge_value.cc',
      'common/android/gin_java_bridge_value.h',
      'common/android/hash_set.cc',
      'common/android/hash_set.h',
      'common/android/surface_texture_lookup.cc',
      'common/android/surface_texture_lookup.h',
      'common/android/surface_texture_peer.cc',
      'common/android/surface_texture_peer.h',
      'common/appcache_messages.h',
      'common/battery_status_messages.h',
      'common/browser_plugin/browser_plugin_constants.cc',
      'common/browser_plugin/browser_plugin_constants.h',
      'common/browser_plugin/browser_plugin_messages.h',
      'common/cc_messages.cc',
      'common/cc_messages.h',
      'common/child_process_host_impl.cc',
      'common/child_process_host_impl.h',
      'common/child_process_messages.h',
      'common/child_process_sandbox_support_impl_linux.cc',
      'common/child_process_sandbox_support_impl_linux.h',
      'common/child_process_sandbox_support_impl_shm_linux.cc',
      'common/clipboard_format.h',
      'common/clipboard_messages.h',
      'common/content_constants_internal.cc',
      'common/content_constants_internal.h',
      'common/content_export.h',
      'common/content_ipc_logging.cc',
      'common/content_message_generator.cc',
      'common/content_message_generator.h',
      'common/content_param_traits.cc',
      'common/content_param_traits.h',
      'common/content_param_traits_macros.h',
      'common/content_paths.cc',
      'common/content_switches_internal.cc',
      'common/content_switches_internal.h',
      'common/cookie_data.cc',
      'common/cookie_data.h',
      'common/cursors/webcursor.cc',
      'common/cursors/webcursor.h',
      'common/cursors/webcursor_android.cc',
      'common/cursors/webcursor_aura.cc',
      'common/cursors/webcursor_aurawin.cc',
      'common/cursors/webcursor_aurax11.cc',
      'common/cursors/webcursor_mac.mm',
      'common/cursors/webcursor_ozone.cc',
      'common/database_messages.h',
      'common/date_time_suggestion.h',
      'common/desktop_notification_messages.h',
      'common/device_sensors/device_motion_hardware_buffer.h',
      'common/device_sensors/device_motion_messages.h',
      'common/device_sensors/device_orientation_hardware_buffer.h',
      'common/device_sensors/device_orientation_messages.h',
      'common/devtools_messages.h',
      'common/dom_storage/dom_storage_map.cc',
      'common/dom_storage/dom_storage_map.h',
      'common/dom_storage/dom_storage_messages.h',
      'common/drag_event_source_info.h',
      'common/drag_messages.h',
      'common/drag_traits.h',
      'common/edit_command.h',
      'common/file_utilities_messages.h',
      'common/fileapi/file_system_messages.h',
      'common/fileapi/webblob_messages.h',
      'common/font_cache_dispatcher_win.cc',
      'common/font_cache_dispatcher_win.h',
      'common/font_config_ipc_linux.cc',
      'common/font_config_ipc_linux.h',
      'common/font_list.cc',
      'common/font_list.h',
      'common/font_list_android.cc',
      'common/font_list_mac.mm',
      'common/font_list_ozone.cc',
      'common/font_list_pango.cc',
      'common/font_list_win.cc',
      'common/frame_message_enums.h',
      'common/frame_messages.h',
      'common/frame_param.cc',
      'common/frame_param.h',
      'common/frame_param_macros.h',
      'common/gamepad_hardware_buffer.h',
      'common/gamepad_messages.h',
      'common/gamepad_param_traits.cc',
      'common/gamepad_param_traits.h',
      'common/gamepad_user_gesture.cc',
      'common/gamepad_user_gesture.h',
      'common/geolocation_messages.h',
      'common/gin_java_bridge_messages.h',
      'common/gpu/client/command_buffer_proxy_impl.cc',
      'common/gpu/client/command_buffer_proxy_impl.h',
      'common/gpu/client/context_provider_command_buffer.cc',
      'common/gpu/client/context_provider_command_buffer.h',
      'common/gpu/client/gl_helper.cc',
      'common/gpu/client/gl_helper.h',
      'common/gpu/client/gl_helper_readback_support.cc',
      'common/gpu/client/gl_helper_readback_support.h',
      'common/gpu/client/gl_helper_scaling.cc',
      'common/gpu/client/gl_helper_scaling.h',
      'common/gpu/client/gpu_channel_host.cc',
      'common/gpu/client/gpu_channel_host.h',
      'common/gpu/client/gpu_memory_buffer_factory_host.h',
      'common/gpu/client/gpu_memory_buffer_impl.cc',
      'common/gpu/client/gpu_memory_buffer_impl.h',
      'common/gpu/client/gpu_memory_buffer_impl_android.cc',
      'common/gpu/client/gpu_memory_buffer_impl_linux.cc',
      'common/gpu/client/gpu_memory_buffer_impl_mac.cc',
      'common/gpu/client/gpu_memory_buffer_impl_shm.cc',
      'common/gpu/client/gpu_memory_buffer_impl_shm.h',
      'common/gpu/client/gpu_memory_buffer_impl_win.cc',
      'common/gpu/client/gpu_video_decode_accelerator_host.cc',
      'common/gpu/client/gpu_video_decode_accelerator_host.h',
      'common/gpu/client/gpu_video_encode_accelerator_host.cc',
      'common/gpu/client/gpu_video_encode_accelerator_host.h',
      'common/gpu/client/webgraphicscontext3d_command_buffer_impl.cc',
      'common/gpu/client/webgraphicscontext3d_command_buffer_impl.h',
      'common/gpu/devtools_gpu_agent.cc',
      'common/gpu/devtools_gpu_agent.h',
      'common/gpu/devtools_gpu_instrumentation.cc',
      'common/gpu/devtools_gpu_instrumentation.h',
      'common/gpu/gpu_channel.cc',
      'common/gpu/gpu_channel.h',
      'common/gpu/gpu_channel_manager.cc',
      'common/gpu/gpu_channel_manager.h',
      'common/gpu/gpu_command_buffer_stub.cc',
      'common/gpu/gpu_command_buffer_stub.h',
      'common/gpu/gpu_config.h',
      'common/gpu/gpu_memory_manager.cc',
      'common/gpu/gpu_memory_manager.h',
      'common/gpu/gpu_memory_manager_client.cc',
      'common/gpu/gpu_memory_manager_client.h',
      'common/gpu/gpu_memory_tracking.cc',
      'common/gpu/gpu_memory_tracking.h',
      'common/gpu/gpu_memory_uma_stats.h',
      'common/gpu/gpu_messages.h',
      'common/gpu/gpu_process_launch_causes.h',
      'common/gpu/gpu_result_codes.h',
      'common/gpu/gpu_surface_lookup.cc',
      'common/gpu/gpu_surface_lookup.h',
      'common/gpu/gpu_watchdog.h',
      'common/gpu/image_transport_surface.cc',
      'common/gpu/image_transport_surface.h',
      'common/gpu/image_transport_surface_android.cc',
      'common/gpu/image_transport_surface_fbo_mac.cc',
      'common/gpu/image_transport_surface_fbo_mac.h',
      'common/gpu/image_transport_surface_linux.cc',
      'common/gpu/image_transport_surface_mac.mm',
      'common/gpu/image_transport_surface_iosurface_mac.cc',
      'common/gpu/image_transport_surface_iosurface_mac.h',
      'common/gpu/image_transport_surface_win.cc',
      'common/gpu/media/gpu_video_decode_accelerator.cc',
      'common/gpu/media/gpu_video_decode_accelerator.h',
      'common/gpu/media/gpu_video_encode_accelerator.cc',
      'common/gpu/media/gpu_video_encode_accelerator.h',
      'common/gpu/stream_texture_android.cc',
      'common/gpu/stream_texture_android.h',
      'common/gpu/sync_point_manager.cc',
      'common/gpu/sync_point_manager.h',
      'common/gpu/texture_image_transport_surface.cc',
      'common/gpu/texture_image_transport_surface.h',
      'common/handle_enumerator_win.cc',
      'common/handle_enumerator_win.h',
      'common/host_shared_bitmap_manager.cc',
      'common/host_shared_bitmap_manager.h',
      'common/image_messages.h',
      'common/indexed_db/indexed_db_constants.h',
      'common/indexed_db/indexed_db_key.cc',
      'common/indexed_db/indexed_db_key.h',
      'common/indexed_db/indexed_db_key_path.cc',
      'common/indexed_db/indexed_db_key_path.h',
      'common/indexed_db/indexed_db_key_range.cc',
      'common/indexed_db/indexed_db_key_range.h',
      'common/indexed_db/indexed_db_messages.h',
      'common/indexed_db/indexed_db_param_traits.cc',
      'common/indexed_db/indexed_db_param_traits.h',
      'common/input/did_overscroll_params.h',
      'common/input/gesture_event_stream_validator.cc',
      'common/input/gesture_event_stream_validator.h',
      'common/input/input_event.cc',
      'common/input/input_event.h',
      'common/input/input_event_stream_validator.cc',
      'common/input/input_event_stream_validator.h',
      'common/input/input_param_traits.cc',
      'common/input/input_param_traits.h',
      'common/input/scoped_web_input_event.cc',
      'common/input/scoped_web_input_event.h',
      'common/input/synthetic_gesture_packet.cc',
      'common/input/synthetic_gesture_packet.h',
      'common/input/synthetic_gesture_params.cc',
      'common/input/synthetic_gesture_params.h',
      'common/input/synthetic_pinch_gesture_params.cc',
      'common/input/synthetic_pinch_gesture_params.h',
      'common/input/synthetic_smooth_scroll_gesture_params.cc',
      'common/input/synthetic_smooth_scroll_gesture_params.h',
      'common/input/synthetic_tap_gesture_params.cc',
      'common/input/synthetic_tap_gesture_params.h',
      'common/input/synthetic_web_input_event_builders.cc',
      'common/input/synthetic_web_input_event_builders.h',
      'common/input/touch_event_stream_validator.cc',
      'common/input/touch_event_stream_validator.h',
      'common/input/web_input_event_traits.cc',
      'common/input/web_input_event_traits.h',
      'common/input/web_touch_event_traits.cc',
      'common/input/web_touch_event_traits.h',
      'common/input_messages.h',
      'common/inter_process_time_ticks_converter.cc',
      'common/inter_process_time_ticks_converter.h',
      'common/java_bridge_messages.h',
      'common/mac/attributed_string_coder.h',
      'common/mac/attributed_string_coder.mm',
      'common/mac/font_descriptor.h',
      'common/mac/font_descriptor.mm',
      'common/mac/font_loader.h',
      'common/mac/font_loader.mm',
      'common/media/aec_dump_messages.h',
      'common/media/audio_messages.h',
      'common/media/cdm_messages.h',
      'common/media/cdm_messages_enums.h',
      'common/media/media_param_traits.cc',
      'common/media/media_param_traits.h',
      'common/media/media_player_messages_android.h',
      'common/media/media_player_messages_enums_android.h',
      'common/media/media_stream_messages.h',
      'common/media/media_stream_options.cc',
      'common/media/media_stream_options.h',
      'common/media/media_stream_track_metrics_host_messages.h',
      'common/media/midi_messages.h',
      'common/media/video_capture.h',
      'common/media/video_capture_messages.h',
      'common/media/webrtc_identity_messages.h',
      'common/memory_benchmark_messages.h',
      'common/message_port_messages.h',
      'common/message_router.cc',
      'common/message_router.h',
      'common/mime_registry_messages.h',
      'common/mojo/mojo_messages.h',
      'common/mojo/mojo_service_names.cc',
      'common/mojo/mojo_service_names.h',
      'common/navigation_gesture.h',
      'common/net/url_fetcher.cc',
      'common/net/url_request_user_data.cc',
      'common/net/url_request_user_data.h',
      'common/one_writer_seqlock.cc',
      'common/one_writer_seqlock.h',
      'common/p2p_messages.h',
      'common/page_state_serialization.cc',
      'common/page_state_serialization.h',
      'common/page_zoom.cc',
      'common/pepper_messages.h',
      'common/pepper_plugin_list.cc',
      'common/pepper_plugin_list.h',
      'common/pepper_renderer_instance_data.cc',
      'common/pepper_renderer_instance_data.h',
      'common/plugin_carbon_interpose_constants_mac.cc',
      'common/plugin_carbon_interpose_constants_mac.h',
      'common/plugin_constants_win.cc',
      'common/plugin_constants_win.h',
      'common/plugin_list.cc',
      'common/plugin_list.h',
      'common/plugin_list_mac.mm',
      'common/plugin_list_posix.cc',
      'common/plugin_list_win.cc',
      'common/plugin_process_messages.h',
      'common/power_monitor_messages.h',
      'common/process_type.cc',
      'common/push_messaging_messages.h',
      'common/quota_messages.h',
      'common/resource_messages.cc',
      'common/resource_messages.h',
      'common/resource_request_body.cc',
      'common/resource_request_body.h',
      'common/sandbox_init_mac.cc',
      'common/sandbox_init_mac.h',
      'common/sandbox_init_win.cc',
      'common/sandbox_linux/android/sandbox_bpf_base_policy_android.cc',
      'common/sandbox_linux/android/sandbox_bpf_base_policy_android.h',
      'common/sandbox_linux/bpf_cros_arm_gpu_policy_linux.cc',
      'common/sandbox_linux/bpf_cros_arm_gpu_policy_linux.h',
      'common/sandbox_linux/bpf_gpu_policy_linux.cc',
      'common/sandbox_linux/bpf_gpu_policy_linux.h',
      'common/sandbox_linux/bpf_ppapi_policy_linux.cc',
      'common/sandbox_linux/bpf_ppapi_policy_linux.h',
      'common/sandbox_linux/bpf_renderer_policy_linux.cc',
      'common/sandbox_linux/bpf_renderer_policy_linux.h',
      'common/sandbox_linux/sandbox_bpf_base_policy_linux.cc',
      'common/sandbox_linux/sandbox_bpf_base_policy_linux.h',
      'common/sandbox_linux/sandbox_init_linux.cc',
      'common/sandbox_linux/sandbox_linux.cc',
      'common/sandbox_linux/sandbox_linux.h',
      'common/sandbox_linux/sandbox_seccomp_bpf_linux.cc',
      'common/sandbox_linux/sandbox_seccomp_bpf_linux.h',
      'common/sandbox_mac.h',
      'common/sandbox_mac.mm',
      'common/sandbox_util.cc',
      'common/sandbox_util.h',
      'common/sandbox_win.cc',
      'common/sandbox_win.h',
      'common/savable_url_schemes.cc',
      'common/savable_url_schemes.h',
      'common/screen_orientation_messages.h',
      'common/service_worker/embedded_worker_messages.h',
      'common/service_worker/service_worker_messages.h',
      'common/service_worker/service_worker_status_code.cc',
      'common/service_worker/service_worker_status_code.h',
      'common/service_worker/service_worker_types.cc',
      'common/service_worker/service_worker_types.h',
      'common/set_process_title.cc',
      'common/set_process_title.h',
      'common/set_process_title_linux.cc',
      'common/set_process_title_linux.h',
      'common/socket_stream.h',
      'common/socket_stream_handle_data.h',
      'common/socket_stream_messages.h',
      'common/speech_recognition_messages.h',
      'common/ssl_status_serialization.cc',
      'common/ssl_status_serialization.h',
      'common/swapped_out_messages.cc',
      'common/swapped_out_messages.h',
      'common/text_input_client_messages.h',
      'common/url_schemes.cc',
      'common/url_schemes.h',
      'common/user_agent.cc',
      'common/user_agent_ios.mm',
      'common/utility_messages.h',
      'common/view_message_enums.h',
      'common/view_messages.h',
      'common/webplugin_geometry.cc',
      'common/webplugin_geometry.h',
      'common/websocket.cc',
      'common/websocket.h',
      'common/websocket_messages.h',
      'common/worker_messages.h',
      'common/zygote_commands_linux.h',
    ],
  },
  'sources': [
    '<@(public_common_sources)',
    '<@(private_common_sources)',
  ],
  'target_conditions': [
    ['OS=="android" and <(use_seccomp_bpf)==1', {
      'sources/': [
        ['include', '^common/sandbox_linux/sandbox_bpf_base_policy_linux\\.cc$'],
        ['include', '^common/sandbox_linux/sandbox_bpf_base_policy_linux\\.h$'],
      ],
    }],
  ],
  'conditions': [
    ['OS=="ios"', {
      # iOS has different user-agent construction utilities, since the
      # version strings is not derived from webkit_version, and follows
      # a different format.
      'sources!': [
        'common/user_agent.cc',
      ],
      'sources/': [
        # iOS only needs a small portion of content; exclude all the
        # implementation, and re-include what is used.
        ['exclude', '\\.(cc|mm)$'],
        ['include', '_ios\\.(cc|mm)$'],
        ['include', '^public/common/content_client\\.cc$'],
        ['include', '^public/common/content_constants\\.cc$'],
        ['include', '^public/common/content_switches\\.cc$'],
        ['include', '^public/common/frame_navigate_params\\.cc$'],
        ['include', '^public/common/media_stream_request\\.cc$'],
        ['include', '^public/common/page_state\\.cc$'],
        ['include', '^public/common/page_transition_types\\.cc$'],
        ['include', '^public/common/password_form\\.cc$'],
        ['include', '^public/common/signed_certificate_timestamp_id_and_status\\.cc$'],
        ['include', '^public/common/speech_recognition_result\\.cc$'],
        ['include', '^public/common/ssl_status\\.cc$'],
        ['include', '^public/common/url_constants\\.cc$'],
        ['include', '^common/content_paths\\.cc$'],
        ['include', '^common/media/media_stream_options\\.cc$'],
        ['include', '^common/net/url_fetcher\\.cc$'],
        ['include', '^common/net/url_request_user_data\\.cc$'],
        ['include', '^common/page_state_serialization\\.cc$'],
        ['include', '^common/savable_url_schemes\\.cc$'],
        ['include', '^common/url_schemes\\.cc$'],
      ],
    }, {  # OS!="ios"
      'dependencies': [
        '../cc/cc.gyp:cc',
        '../gpu/gpu.gyp:command_buffer_service',
        '../gpu/gpu.gyp:gles2_c_lib',
        '../gpu/gpu.gyp:gles2_implementation',
        # TODO: the dependency on gl_in_process_context should be decoupled from
        # content and moved to android_webview. See crbug.com/365797.
        '../gpu/gpu.gyp:gl_in_process_context',
        '../gpu/gpu.gyp:gpu_ipc',
        '../gpu/skia_bindings/skia_bindings.gyp:gpu_skia_bindings',
        '../ipc/ipc.gyp:ipc',
        '../media/media.gyp:media',
        '../media/media.gyp:shared_memory_support',
        '../mojo/mojo.gyp:mojo_environment_chromium',
        '../mojo/mojo.gyp:mojo_system_impl',
        '../third_party/WebKit/public/blink.gyp:blink',
        '../ui/gl/gl.gyp:gl',
        '../webkit/common/gpu/webkit_gpu.gyp:webkit_gpu',
        '../webkit/common/webkit_common.gyp:webkit_common',
        '../webkit/storage_browser.gyp:webkit_storage_browser',
        '../webkit/storage_common.gyp:webkit_storage_common',
      ],
      'actions': [
        {
          'action_name': 'generate_webkit_version',
          'inputs': [
            '<(script)',
            '<(lastchange)',
            '<(template)',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/webkit_version.h',
          ],
          'action': ['python',
                     '<(script)',
                     '-f', '<(lastchange)',
                     '<(template)',
                     '<@(_outputs)',
          ],
          'variables': {
            'script': '<(DEPTH)/build/util/version.py',
            'lastchange': '<(DEPTH)/build/util/LASTCHANGE.blink',
            'template': 'webkit_version.h.in',
          },
        },
      ],
    }],
    ['OS=="mac"', {
      'dependencies': [
        '../webkit/webkit_resources.gyp:webkit_resources',
      ],
      'sources': [
        'common/gpu/client/gpu_memory_buffer_impl_io_surface.cc',
        'common/gpu/client/gpu_memory_buffer_impl_io_surface.h',
        'common/gpu/media/vt_video_decode_accelerator.cc',
        'common/gpu/media/vt_video_decode_accelerator.h',
      ],
      'sources!': [
        'common/plugin_list_posix.cc',
      ],
      'link_settings': {
        'libraries': [
          '$(SDKROOT)/System/Library/Frameworks/IOSurface.framework',
          '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
        ],
      },
    }],
    ['OS=="android"',{
      'sources': [
        'common/gpu/client/gpu_memory_buffer_impl_surface_texture.cc',
        'common/gpu/client/gpu_memory_buffer_impl_surface_texture.h',
      ],
      'link_settings': {
        'libraries': [
          '-landroid',  # ANativeWindow
        ],
      },
     'dependencies': [
        'content.gyp:content_jni_headers',
        'content.gyp:common_aidl',
      ],
    }],
    ['use_pango == 1', {
      'dependencies': [
        '../build/linux/system.gyp:pangocairo',
      ],
      'sources!': [
        'common/font_list_ozone.cc',
      ],
    }],
    ['use_x11 == 1', {
      'include_dirs': [
        '<(DEPTH)/third_party/khronos',
      ],
      'dependencies': [
         '<(DEPTH)/build/linux/system.gyp:xcomposite',
      ],
    }],
    ['use_x11 == 1 and (target_arch != "arm" or chromeos == 0)', {
      'sources': [
        'common/gpu/x_util.cc',
        'common/gpu/x_util.h',
      ],
    }],
    ['enable_plugins==1', {
      'dependencies': [
        '../ppapi/ppapi_internal.gyp:ppapi_shared',
      ],
    }, {  # enable_plugins == 0
      'sources!': [
        'common/pepper_plugin_list.cc',
        'common/pepper_plugin_list.h',
        'common/sandbox_util.cc',
      ],
    }],
    ['OS=="android"', {
      'dependencies': [
        '../media/media.gyp:media',
      ],
      'sources': [
        'common/gpu/media/android_video_decode_accelerator.cc',
        'common/gpu/media/android_video_decode_accelerator.h',
      ],
    }],
    ['OS=="android" and enable_webrtc==1', {
      'dependencies': [
        '../third_party/libyuv/libyuv.gyp:libyuv',
      ],
      'sources': [
        'common/gpu/media/android_video_encode_accelerator.cc',
        'common/gpu/media/android_video_encode_accelerator.h',
      ],
    }],
    ['target_arch=="arm" and chromeos == 1 and use_x11 == 1', {
      'dependencies': [
        '../media/media.gyp:media',
      ],
      'sources': [
        'common/gpu/media/exynos_v4l2_video_device.cc',
        'common/gpu/media/exynos_v4l2_video_device.h',
        'common/gpu/media/tegra_v4l2_video_device.cc',
        'common/gpu/media/tegra_v4l2_video_device.h',
        'common/gpu/media/v4l2_image_processor.cc',
        'common/gpu/media/v4l2_image_processor.h',
        'common/gpu/media/v4l2_video_decode_accelerator.cc',
        'common/gpu/media/v4l2_video_decode_accelerator.h',
        'common/gpu/media/v4l2_video_device.cc',
        'common/gpu/media/v4l2_video_device.h',
        'common/gpu/media/v4l2_video_encode_accelerator.cc',
        'common/gpu/media/v4l2_video_encode_accelerator.h',
      ],
      'include_dirs': [
        '<(DEPTH)/third_party/khronos',
      ],
    }],
    ['target_arch != "arm" and chromeos == 1 and use_x11 == 1', {
      'sources': [
        'common/gpu/media/h264_dpb.cc',
        'common/gpu/media/h264_dpb.h',
        'common/gpu/media/va_surface.h',
        'common/gpu/media/vaapi_h264_decoder.cc',
        'common/gpu/media/vaapi_h264_decoder.h',
        'common/gpu/media/vaapi_video_decode_accelerator.cc',
        'common/gpu/media/vaapi_video_decode_accelerator.h',
        'common/gpu/media/vaapi_wrapper.cc',
        'common/gpu/media/vaapi_wrapper.h',
      ],
      'variables': {
        'generate_stubs_script': '../tools/generate_stubs/generate_stubs.py',
        'extra_header': 'common/gpu/media/va_stub_header.fragment',
        'sig_files': ['common/gpu/media/va.sigs'],
        'outfile_type': 'posix_stubs',
        'stubs_filename_root': 'va_stubs',
        'project_path': 'content/common/gpu/media',
        'intermediate_dir': '<(INTERMEDIATE_DIR)',
        'output_root': '<(SHARED_INTERMEDIATE_DIR)/va',
      },
      'include_dirs': [
        '<(DEPTH)/third_party/libva',
        '<(output_root)',
      ],
      'actions': [
        {
          'action_name': 'generate_stubs',
          'inputs': [
            '<(generate_stubs_script)',
            '<(extra_header)',
            '<@(sig_files)',
          ],
          'outputs': [
            '<(intermediate_dir)/<(stubs_filename_root).cc',
            '<(output_root)/<(project_path)/<(stubs_filename_root).h',
          ],
          'action': ['python',
                     '<(generate_stubs_script)',
                     '-i', '<(intermediate_dir)',
                     '-o', '<(output_root)/<(project_path)',
                     '-t', '<(outfile_type)',
                     '-e', '<(extra_header)',
                     '-s', '<(stubs_filename_root)',
                     '-p', '<(project_path)',
                     '<@(_inputs)',
          ],
          'process_outputs_as_sources': 1,
          'message': 'Generating libva stubs for dynamic loading',
        },
     ]
    }],
    ['OS=="win"', {
      'dependencies': [
        '../media/media.gyp:media',
        '../ui/gl/gl.gyp:gl',
      ],
      'link_settings': {
        'libraries': [
           '-ld3d9.lib',
           '-ldxva2.lib',
           '-lstrmiids.lib',
           '-lmf.lib',
           '-lmfplat.lib',
           '-lmfuuid.lib',
        ],
        'msvs_settings': {
          'VCLinkerTool': {
            'DelayLoadDLLs': [
              'd3d9.dll',
              'dxva2.dll',
              'mf.dll',
              'mfplat.dll',
            ],
          },
        },
      },
      'sources': [
        'common/gpu/media/dxva_video_decode_accelerator.cc',
        'common/gpu/media/dxva_video_decode_accelerator.h',
      ],
      'include_dirs': [
        '<(DEPTH)/third_party/khronos',
      ],
    }],
    ['OS=="win" and directxsdk_exists=="True"', {
      'actions': [
      {
        'action_name': 'extract_xinput',
        'variables': {
          'input': 'APR2007_xinput_<(winsdk_arch).cab',
          'output': 'xinput1_3.dll',
        },
        'inputs': [
          '../third_party/directxsdk/files/Redist/<(input)',
        ],
        'outputs': [
          '<(PRODUCT_DIR)/<(output)',
        ],
        'action': [
          'python',
        '../build/extract_from_cab.py',
        '..\\third_party\\directxsdk\\files\\Redist\\<(input)',
        '<(output)',
        '<(PRODUCT_DIR)',
        ],
      },
     ]
    }],
    ['use_seccomp_bpf==0', {
      'sources!': [
        'common/sandbox_linux/android/sandbox_bpf_base_policy_android.cc',
        'common/sandbox_linux/android/sandbox_bpf_base_policy_android.h',
        'common/sandbox_linux/bpf_cros_arm_gpu_policy_linux.cc',
        'common/sandbox_linux/bpf_cros_arm_gpu_policy_linux.h',
        'common/sandbox_linux/bpf_gpu_policy_linux.cc',
        'common/sandbox_linux/bpf_gpu_policy_linux.h',
        'common/sandbox_linux/bpf_ppapi_policy_linux.cc',
        'common/sandbox_linux/bpf_ppapi_policy_linux.h',
        'common/sandbox_linux/bpf_renderer_policy_linux.cc',
        'common/sandbox_linux/bpf_renderer_policy_linux.h',
        'common/sandbox_linux/sandbox_bpf_base_policy_linux.cc',
        'common/sandbox_linux/sandbox_bpf_base_policy_linux.h',
      ],
    }, {
      'defines': ['USE_SECCOMP_BPF'],
    }],
    ['use_ozone==1', {
      'dependencies': [
        '../ui/ozone/ozone.gyp:ozone',
      ],
    }],
  ],
}
