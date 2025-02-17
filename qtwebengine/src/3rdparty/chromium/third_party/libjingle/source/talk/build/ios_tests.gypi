#
# libjingle
# Copyright 2014, Google Inc.
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

# Include this .gypi in an ObjC target's definition to allow it to be
# used as an iOS or OS/X application.

{
  'conditions': [
    ['OS=="ios"', {
      'variables': {
        'infoplist_file': './ios_test.plist',
      },
      'mac_bundle': 1,
      'mac_bundle_resources': [
        '<(infoplist_file)',
      ],
      # The plist is listed above so that it appears in XCode's file list,
      # but we don't actually want to bundle it.
      'mac_bundle_resources!': [
        '<(infoplist_file)',
      ],
      'xcode_settings': {
        'CLANG_ENABLE_OBJC_ARC': 'YES',
        # common.gypi enables this for mac but we want this to be disabled
        # like it is for ios.
        'CLANG_WARN_OBJC_MISSING_PROPERTY_SYNTHESIS': 'NO',
        'INFOPLIST_FILE': '<(infoplist_file)',
      },
    }],
  ],  # conditions
}
