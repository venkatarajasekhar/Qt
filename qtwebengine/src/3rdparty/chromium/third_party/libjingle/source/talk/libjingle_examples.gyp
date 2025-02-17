#
# libjingle
# Copyright 2012, Google Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#  3. The name of the author may not be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

{
  'includes': [
    'build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'libjingle_xmpphelp',
      'type': 'static_library',
      'dependencies': [
        '<(DEPTH)/third_party/expat/expat.gyp:expat',
        'libjingle.gyp:libjingle',
        'libjingle.gyp:libjingle_p2p',
      ],
      'sources': [
        'xmpp/jingleinfotask.cc',
        'xmpp/jingleinfotask.h',
      ],
    },  # target libjingle_xmpphelp
    {
      'target_name': 'relayserver',
      'type': 'executable',
      'dependencies': [
        'libjingle.gyp:libjingle',
        'libjingle.gyp:libjingle_p2p',
      ],
      'sources': [
        'examples/relayserver/relayserver_main.cc',
      ],
    },  # target relayserver
    {
      'target_name': 'stunserver',
      'type': 'executable',
      'dependencies': [
        'libjingle.gyp:libjingle',
        'libjingle.gyp:libjingle_p2p',
      ],
      'sources': [
        'examples/stunserver/stunserver_main.cc',
      ],
    },  # target stunserver
    {
      'target_name': 'turnserver',
      'type': 'executable',
      'dependencies': [
        'libjingle.gyp:libjingle',
        'libjingle.gyp:libjingle_p2p',
      ],
      'sources': [
        'examples/turnserver/turnserver_main.cc',
      ],
    },  # target turnserver
    {
      'target_name': 'login',
      'type': 'executable',
      'dependencies': [
        'libjingle_xmpphelp',
      ],
      'sources': [
        'examples/login/login_main.cc',
      ],
    },  # target login
    {
      'target_name': 'peerconnection_server',
      'type': 'executable',
      'sources': [
        'examples/peerconnection/server/data_socket.cc',
        'examples/peerconnection/server/data_socket.h',
        'examples/peerconnection/server/main.cc',
        'examples/peerconnection/server/peer_channel.cc',
        'examples/peerconnection/server/peer_channel.h',
        'examples/peerconnection/server/utils.cc',
        'examples/peerconnection/server/utils.h',
      ],
      'dependencies': [
        'libjingle.gyp:libjingle',
      ],
      # TODO(ronghuawu): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4309, ],
    }, # target peerconnection_server
  ],
  'conditions': [
    # TODO(ronghuawu): Reenable building call.
    # ['OS!="android"', {
    #   'targets': [
    #     {
    #       'target_name': 'call',
    #       'type': 'executable',
    #       'dependencies': [
    #         'libjingle.gyp:libjingle_p2p',
    #         'libjingle_xmpphelp',
    #       ],
    #       'sources': [
    #         'examples/call/call_main.cc',
    #         'examples/call/callclient.cc',
    #         'examples/call/callclient.h',
    #         'examples/call/console.cc',
    #         'examples/call/console.h',
    #         'examples/call/friendinvitesendtask.cc',
    #         'examples/call/friendinvitesendtask.h',
    #         'examples/call/mediaenginefactory.cc',
    #         'examples/call/mediaenginefactory.h',
    #         'examples/call/muc.h',
    #         'examples/call/mucinviterecvtask.cc',
    #         'examples/call/mucinviterecvtask.h',
    #         'examples/call/mucinvitesendtask.cc',
    #         'examples/call/mucinvitesendtask.h',
    #         'examples/call/presencepushtask.cc',
    #         'examples/call/presencepushtask.h',
    #       ],
    #       'conditions': [
    #         ['OS=="linux"', {
    #           'link_settings': {
    #             'libraries': [
    #               '<!@(pkg-config --libs-only-l gobject-2.0 gthread-2.0'
    #                   ' gtk+-2.0)',
    #             ],
    #           },
    #         }],
    #         ['OS=="win"', {
    #           'msvs_settings': {
    #             'VCLinkerTool': {
    #               'AdditionalDependencies': [
    #                 'strmiids.lib',
    #               ],
    #             },
    #           },
    #         }],
    #       ],  # conditions
    #     },  # target call
    #   ], # targets
    # }],  # OS!="android"
    ['OS=="linux" or OS=="win"', {
      'targets': [
        {
          'target_name': 'peerconnection_client',
          'type': 'executable',
          'sources': [
            'examples/peerconnection/client/conductor.cc',
            'examples/peerconnection/client/conductor.h',
            'examples/peerconnection/client/defaults.cc',
            'examples/peerconnection/client/defaults.h',
            'examples/peerconnection/client/peer_connection_client.cc',
            'examples/peerconnection/client/peer_connection_client.h',
          ],
          'dependencies': [
            '<(DEPTH)/third_party/jsoncpp/jsoncpp.gyp:jsoncpp',
            'libjingle.gyp:libjingle_peerconnection',
          ],
          'conditions': [
            # TODO(ronghuawu): Move these files to a win/ directory then they
            # can be excluded automatically.
            ['OS=="win"', {
              'sources': [
                'examples/peerconnection/client/flagdefs.h',
                'examples/peerconnection/client/main.cc',
                'examples/peerconnection/client/main_wnd.cc',
                'examples/peerconnection/client/main_wnd.h',
              ],
              'msvs_settings': {
                'VCLinkerTool': {
                 'SubSystem': '2',  # Windows
                },
              },
            }],  # OS=="win"
            ['OS=="linux"', {
              'sources': [
                'examples/peerconnection/client/linux/main.cc',
                'examples/peerconnection/client/linux/main_wnd.cc',
                'examples/peerconnection/client/linux/main_wnd.h',
              ],
              'cflags': [
                '<!@(pkg-config --cflags glib-2.0 gobject-2.0 gtk+-2.0)',
              ],
              'link_settings': {
                'ldflags': [
                  '<!@(pkg-config --libs-only-L --libs-only-other glib-2.0'
                      ' gobject-2.0 gthread-2.0 gtk+-2.0)',
                ],
                'libraries': [
                  '<!@(pkg-config --libs-only-l glib-2.0 gobject-2.0'
                      ' gthread-2.0 gtk+-2.0)',
                  '-lX11',
                  '-lXcomposite',
                  '-lXext',
                  '-lXrender',
                ],
              },
            }],  # OS=="linux"
          ],  # conditions
        },  # target peerconnection_client
      ], # targets
    }],  # OS=="linux" or OS=="win"

    ['OS=="ios" or (OS=="mac" and target_arch!="ia32" and mac_sdk>="10.8")', {
      'targets': [
        {
          'target_name': 'AppRTCDemo',
          'type': 'executable',
          'product_name': 'AppRTCDemo',
          'mac_bundle': 1,
          'mac_bundle_resources': [
            'examples/objc/AppRTCDemo/channel.html',
          ],
          'dependencies': [
            'libjingle.gyp:libjingle_peerconnection_objc',
          ],
          'conditions': [
            ['OS=="ios"', {
              'mac_bundle_resources': [
                'examples/objc/AppRTCDemo/ios/ResourceRules.plist',
                'examples/objc/AppRTCDemo/ios/en.lproj/APPRTCViewController.xib',
                'examples/objc/Icon.png',
              ],
              'sources': [
                'examples/objc/AppRTCDemo/ios/APPRTCAppDelegate.h',
                'examples/objc/AppRTCDemo/ios/APPRTCAppDelegate.m',
                'examples/objc/AppRTCDemo/ios/APPRTCViewController.h',
                'examples/objc/AppRTCDemo/ios/APPRTCViewController.m',
                'examples/objc/AppRTCDemo/ios/AppRTCDemo-Prefix.pch',
                'examples/objc/AppRTCDemo/ios/main.m',
              ],
              'xcode_settings': {
                'INFOPLIST_FILE': 'examples/objc/AppRTCDemo/ios/Info.plist',
              },
            }],
            ['OS=="mac"', {
              'sources': [
                'examples/objc/AppRTCDemo/mac/APPRTCAppDelegate.h',
                'examples/objc/AppRTCDemo/mac/APPRTCAppDelegate.m',
                'examples/objc/AppRTCDemo/mac/APPRTCViewController.h',
                'examples/objc/AppRTCDemo/mac/APPRTCViewController.m',
                'examples/objc/AppRTCDemo/mac/main.m',
              ],
              'xcode_settings': {
                'CLANG_WARN_OBJC_MISSING_PROPERTY_SYNTHESIS': 'NO',
                'INFOPLIST_FILE': 'examples/objc/AppRTCDemo/mac/Info.plist',
                'MACOSX_DEPLOYMENT_TARGET' : '10.8',
                'OTHER_LDFLAGS': [
                  '-framework AVFoundation',
                  '-framework WebKit',
                ],
              },
            }],
            ['target_arch=="ia32"', {
              'dependencies' : [
                '<(DEPTH)/testing/iossim/iossim.gyp:iossim#host',
              ],
            }],
          ],
          'include_dirs': [
            'examples/objc/APPRTCDemo',
          ],
          'sources': [
            'examples/objc/AppRTCDemo/APPRTCAppClient.h',
            'examples/objc/AppRTCDemo/APPRTCAppClient.m',
            'examples/objc/AppRTCDemo/APPRTCConnectionManager.h',
            'examples/objc/AppRTCDemo/APPRTCConnectionManager.m',
            'examples/objc/AppRTCDemo/GAEChannelClient.h',
            'examples/objc/AppRTCDemo/GAEChannelClient.m',
          ],
          'xcode_settings': {
            'CLANG_ENABLE_OBJC_ARC': 'YES',
          },
        },  # target AppRTCDemo
      ],  # targets
    }],  # OS=="ios" or (OS=="mac" and target_arch!="ia32" and mac_sdk>="10.8")

    ['OS=="android"', {
      'targets': [
        {
          'target_name': 'AppRTCDemo',
          'type': 'none',
          'dependencies': [
            'libjingle.gyp:libjingle_peerconnection_jar',
          ],
          'actions': [
            {
              # TODO(fischman): convert from a custom script to a standard gyp
              # apk build once chromium's apk-building gyp machinery can be used
              # (http://crbug.com/225101)
              'action_name': 'build_apprtcdemo_apk',
              'inputs' : [
                '<(PRODUCT_DIR)/libjingle_peerconnection.jar',
                '<(PRODUCT_DIR)/libjingle_peerconnection_so.so',
                'examples/android/AndroidManifest.xml',
                'examples/android/README',
                'examples/android/ant.properties',
                'examples/android/assets/channel.html',
                'examples/android/build.xml',
                'examples/android/jni/Android.mk',
                'examples/android/project.properties',
                'examples/android/res/drawable-hdpi/ic_launcher.png',
                'examples/android/res/drawable-ldpi/ic_launcher.png',
                'examples/android/res/drawable-mdpi/ic_launcher.png',
                'examples/android/res/drawable-xhdpi/ic_launcher.png',
                'examples/android/res/values/strings.xml',
                'examples/android/src/org/appspot/apprtc/AppRTCClient.java',
                'examples/android/src/org/appspot/apprtc/AppRTCDemoActivity.java',
                'examples/android/src/org/appspot/apprtc/AppRTCGLView.java',
                'examples/android/src/org/appspot/apprtc/UnhandledExceptionHandler.java',
                'examples/android/src/org/appspot/apprtc/GAEChannelClient.java',
              ],
              'outputs': [
                '<(PRODUCT_DIR)/AppRTCDemo-debug.apk',
              ],
              'variables': {
                'ant_log': '../../<(INTERMEDIATE_DIR)/ant.log', # ../.. to compensate for the cd examples/android below.
              },
              'action': [
                'bash', '-ec',
                'rm -fr <(_outputs) examples/android/{bin,libs} && '
                'mkdir -p <(INTERMEDIATE_DIR) && ' # Must happen _before_ the cd below
                'mkdir -p examples/android/libs/<(android_app_abi) && '
                'cp <(PRODUCT_DIR)/libjingle_peerconnection.jar examples/android/libs/ &&'
                '<(android_strip) -o examples/android/libs/<(android_app_abi)/libjingle_peerconnection_so.so  <(PRODUCT_DIR)/libjingle_peerconnection_so.so &&'
                'cd examples/android && '
                '{ ANDROID_SDK_ROOT=<(android_sdk_root) '
                'ant debug > <(ant_log) 2>&1 || '
                '  { cat <(ant_log) ; exit 1; } } && '
                'cd - > /dev/null && '
                'cp examples/android/bin/AppRTCDemo-debug.apk <(_outputs)'
              ],
            },
          ],
        },  # target AppRTCDemo
      ],  # targets
    }],  # OS=="android"
  ],
}
