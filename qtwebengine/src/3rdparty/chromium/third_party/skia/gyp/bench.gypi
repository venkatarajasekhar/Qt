{
  'include_dirs': [
    '../src/core',
    '../src/effects',
    '../src/utils',
    '../tools',
  ],
  'dependencies': [
    'skia_lib.gyp:skia_lib',
  ],
  'sources': [
    '../bench/Benchmark.cpp',
    '../bench/Benchmark.h',

    '../bench/AAClipBench.cpp',
    '../bench/BicubicBench.cpp',
    '../bench/BitmapBench.cpp',
    '../bench/BitmapRectBench.cpp',
    '../bench/BitmapScaleBench.cpp',
    '../bench/BlurBench.cpp',
    '../bench/BlurImageFilterBench.cpp',
    '../bench/BlurRectBench.cpp',
    '../bench/BlurRoundRectBench.cpp',
    '../bench/ChartBench.cpp',
    '../bench/ChecksumBench.cpp',
    '../bench/ChromeBench.cpp',
    '../bench/CmapBench.cpp',
    '../bench/ColorFilterBench.cpp',
    '../bench/ColorPrivBench.cpp',
    '../bench/CoverageBench.cpp',
    '../bench/DashBench.cpp',
    '../bench/DecodeBench.cpp',
    '../bench/DeferredCanvasBench.cpp',
    '../bench/DeferredSurfaceCopyBench.cpp',
    '../bench/DisplacementBench.cpp',
    '../bench/ETCBitmapBench.cpp',
    '../bench/FSRectBench.cpp',
    '../bench/FontCacheBench.cpp',
    '../bench/FontScalerBench.cpp',
    '../bench/GameBench.cpp',
    '../bench/GrMemoryPoolBench.cpp',
    '../bench/GrResourceCacheBench.cpp',
    '../bench/GrOrderedSetBench.cpp',
    '../bench/GradientBench.cpp',
    '../bench/HairlinePathBench.cpp',
    '../bench/ImageCacheBench.cpp',
    '../bench/ImageDecodeBench.cpp',
    '../bench/ImageFilterDAGBench.cpp',
    '../bench/InterpBench.cpp',
    '../bench/LightingBench.cpp',
    '../bench/LineBench.cpp',
    '../bench/MagnifierBench.cpp',
    '../bench/MathBench.cpp',
    '../bench/Matrix44Bench.cpp',
    '../bench/MatrixBench.cpp',
    '../bench/MatrixConvolutionBench.cpp',
    '../bench/MemcpyBench.cpp',
    '../bench/MemoryBench.cpp',
    '../bench/MemsetBench.cpp',
    '../bench/MergeBench.cpp',
    '../bench/MorphologyBench.cpp',
    '../bench/MutexBench.cpp',
    '../bench/PathBench.cpp',
    '../bench/PathIterBench.cpp',
    '../bench/PathUtilsBench.cpp',
    '../bench/PerlinNoiseBench.cpp',
    '../bench/PicturePlaybackBench.cpp',
    '../bench/PictureRecordBench.cpp',
    '../bench/PremulAndUnpremulAlphaOpsBench.cpp',
    '../bench/QuadTreeBench.cpp',
    '../bench/RTreeBench.cpp',
    '../bench/ReadPixBench.cpp',
    '../bench/RectBench.cpp',
    '../bench/RectanizerBench.cpp',
    '../bench/RectoriBench.cpp',
    '../bench/RefCntBench.cpp',
    '../bench/RegionBench.cpp',
    '../bench/RegionContainBench.cpp',
    '../bench/RepeatTileBench.cpp',
    '../bench/ScalarBench.cpp',
    '../bench/ShaderMaskBench.cpp',
    '../bench/SkipZeroesBench.cpp',
    '../bench/SortBench.cpp',
    '../bench/StackBench.cpp',
    '../bench/StrokeBench.cpp',
    '../bench/TableBench.cpp',
    '../bench/TextBench.cpp',
    '../bench/TileBench.cpp',
    '../bench/VertBench.cpp',
    '../bench/WritePixelsBench.cpp',
    '../bench/WriterBench.cpp',
    '../bench/XfermodeBench.cpp',
  ],
}
