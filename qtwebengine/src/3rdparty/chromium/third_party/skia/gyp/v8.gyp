# GYP file to build a V8 sample.
{
  'targets': [
    {
      'target_name': 'SkV8Example',
      'type': 'executable',
      'mac_bundle' : 1,
      'include_dirs' : [
        '../tools/flags',
        '../third_party/externals/v8/include',
        ],
       'sources': [
         '../experimental/SkV8Example/SkV8Example.cpp',
         '../experimental/SkV8Example/SkV8Example.h',
         '../experimental/SkV8Example/Global.cpp',
         '../experimental/SkV8Example/Global.h',
         '../experimental/SkV8Example/Path2D.cpp',
         '../experimental/SkV8Example/Path2D.h',
         '../experimental/SkV8Example/BaseContext.cpp',
         '../experimental/SkV8Example/BaseContext.h',
         '../experimental/SkV8Example/JsContext.cpp',
         '../experimental/SkV8Example/JsContext.h',
       ],
       'dependencies': [
         'flags.gyp:flags',
         'skia_lib.gyp:skia_lib',
         'views.gyp:views',
         'xml.gyp:xml',
       ],
       'link_settings': {
         'libraries': [

#        'd:/src/v8/build/Debug/lib/v8_base.ia32.lib',
#        'd:/src/v8/build/Debug/lib/v8_snapshot.lib',
#        'd:/src/v8/build/Debug/lib/icuuc.lib',
#        'd:/src/v8/build/Debug/lib/icui18n.lib',
#        'Ws2_32.lib',
#        'Winmm.lib',

           '-lpthread',
           '-lrt',
           '../../third_party/externals/v8/out/native/obj.target/tools/gyp/libv8_base.x64.a',
           '../../third_party/externals/v8/out/native/obj.target/tools/gyp/libv8_snapshot.a',
           '../../third_party/externals/v8/out/native/obj.target/third_party/icu/libicudata.a',
           '../../third_party/externals/v8/out/native/obj.target/third_party/icu/libicui18n.a',
           '../../third_party/externals/v8/out/native/obj.target/third_party/icu/libicuuc.a',
           '../../third_party/externals/v8/out/native/obj.target/icudata/third_party/icu/linux/icudt46l_dat.o',
           ],
       },
       'conditions' : [
         [ 'skia_gpu == 1', {
           'include_dirs' : [
             '../src/gpu',  #gl/GrGLUtil.h
             ]
         }],
        [ 'skia_os == "win"', {
         'sources' : [
           '../src/views/win/SkOSWindow_Win.cpp',
           '../src/views/win/skia_win.cpp',
           ],
          },
        ],

        [ 'skia_os == "mac"', {
          'sources': [

          '../src/views/mac/SampleAppDelegate.h',
          '../src/views/mac/SampleAppDelegate.mm',
          '../src/views/mac/SkEventNotifier.mm',
          '../src/views/mac/skia_mac.mm',
          '../src/views/mac/SkNSView.h',
          '../src/views/mac/SkNSView.mm',
          '../src/views/mac/SkOptionsTableView.h',
          '../src/views/mac/SkOptionsTableView.mm',
          '../src/views/mac/SkOSWindow_Mac.mm',
          '../src/views/mac/SkTextFieldCell.h',
          '../src/views/mac/SkTextFieldCell.m',
          ],
        'include_dirs' : [
          '../src/views/mac/'
          ],
        'xcode_settings' : {
          'INFOPLIST_FILE' : '../experimental/SkiaExamples/SkiaExamples-Info.plist',
        },
        'mac_bundle_resources' : [
          '../experimental/SkiaExamples/SkiaExamples.xib'
          ],
        }
      ],
     ],
    }
  ],
}
