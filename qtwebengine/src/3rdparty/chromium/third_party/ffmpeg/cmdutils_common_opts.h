    { "L"          , OPT_EXIT, {.func_arg = show_license},      "show license" },
    { "h"          , OPT_EXIT, {.func_arg = show_help},         "show help", "topic" },
    { "?"          , OPT_EXIT, {.func_arg = show_help},         "show help", "topic" },
    { "help"       , OPT_EXIT, {.func_arg = show_help},         "show help", "topic" },
    { "-help"      , OPT_EXIT, {.func_arg = show_help},         "show help", "topic" },
    { "version"    , OPT_EXIT, {.func_arg = show_version},      "show version" },
    { "buildconf"  , OPT_EXIT, {.func_arg = show_buildconf},    "show build configuration" },
    { "formats"    , OPT_EXIT, {.func_arg = show_formats  },    "show available formats" },
    { "codecs"     , OPT_EXIT, {.func_arg = show_codecs   },    "show available codecs" },
    { "decoders"   , OPT_EXIT, {.func_arg = show_decoders },    "show available decoders" },
    { "encoders"   , OPT_EXIT, {.func_arg = show_encoders },    "show available encoders" },
    { "bsfs"       , OPT_EXIT, {.func_arg = show_bsfs     },    "show available bit stream filters" },
    { "protocols"  , OPT_EXIT, {.func_arg = show_protocols},    "show available protocols" },
    { "filters"    , OPT_EXIT, {.func_arg = show_filters  },    "show available filters" },
    { "pix_fmts"   , OPT_EXIT, {.func_arg = show_pix_fmts },    "show available pixel formats" },
    { "layouts"    , OPT_EXIT, {.func_arg = show_layouts  },    "show standard channel layouts" },
    { "sample_fmts", OPT_EXIT, {.func_arg = show_sample_fmts }, "show available audio sample formats" },
    { "colors"     , OPT_EXIT, {.func_arg = show_colors },      "show available color names" },
    { "loglevel"   , HAS_ARG,  {.func_arg = opt_loglevel},      "set logging level", "loglevel" },
    { "v",           HAS_ARG,  {.func_arg = opt_loglevel},      "set logging level", "loglevel" },
    { "report"     , 0,        {(void*)opt_report}, "generate a report" },
    { "max_alloc"  , HAS_ARG,  {.func_arg = opt_max_alloc},     "set maximum size of a single allocated block", "bytes" },
    { "cpuflags"   , HAS_ARG | OPT_EXPERT, { .func_arg = opt_cpuflags }, "force specific cpu flags", "flags" },
    { "hide_banner", OPT_BOOL | OPT_EXPERT, {&hide_banner},     "do not show program banner", "hide_banner" },
#if CONFIG_OPENCL
    { "opencl_bench", OPT_EXIT, {.func_arg = opt_opencl_bench}, "run benchmark on all OpenCL devices and show results" },
    { "opencl_options", HAS_ARG, {.func_arg = opt_opencl},      "set OpenCL environment options" },
#endif
