## OpenSTA
genlex(
    name = "LibExprLex",
    src = "src/sta/liberty/LibExprLex.ll",
    out = "src/sta/liberty/LibExprLex.cc",
    prefix = "LibExprLex_",
)

genyacc(
    name = "LibExprParse",
    src = "src/sta/liberty/LibExprParse.yy",
    header_out = "src/sta/liberty/LibExprParse.hh",
    prefix = "LibExprParse_",
    source_out = "src/sta/liberty/LibExprParse.cc",
)

# Liberty Parser
genlex(
    name = "LibertyLex",
    src = "src/sta/liberty/LibertyLex.ll",
    out = "src/sta/liberty/LibertyLex.cc",
    prefix = "LibertyLex_",
)

genyacc(
    name = "LibertyParser",
    src = "src/sta/liberty/LibertyParse.yy",
    extra_outs = ["src/sta/liberty/LibertyLocation.hh"],
    header_out = "src/sta/liberty/LibertyParse.hh",
    prefix = "LibertyParse_",
    source_out = "src/sta/liberty/LibertyParse.cc",
)

# Spef scan/parse.
genlex(
    name = "SpefLex",
    src = "src/sta/parasitics/SpefLex.ll",
    out = "src/sta/parasitics/SpefLex.cc",
    prefix = "SpefLex_",
)

genyacc(
    name = "SpefParser",
    src = "src/sta/parasitics/SpefParse.yy",
    extra_outs = ["src/sta/parasitics/SpefLocation.hh"],
    header_out = "src/sta/parasitics/SpefParse.hh",
    prefix = "SpefParse_",
    source_out = "src/sta/parasitics/SpefParse.cc",
)

# Verilog scan/parse.
genlex(
    name = "VerilogLex",
    src = "src/sta/verilog/VerilogLex.ll",
    out = "src/sta/verilog/VerilogLex.cc",
    prefix = "VerilogLex_",
)

genyacc(
    name = "VerilogParser",
    src = "src/sta/verilog/VerilogParse.yy",
    extra_outs = ["src/sta/verilog/VerilogLocation.hh"],
    header_out = "src/sta/verilog/VerilogParse.hh",
    prefix = "VerilogParse_",
    source_out = "src/sta/verilog/VerilogParse.cc",
)

# sdf scan/parse.
genlex(
    name = "SdfLex",
    src = "src/sta/sdf/SdfLex.ll",
    out = "src/sta/sdf/SdfLex.cc",
    prefix = "SdfLex_",
)

genyacc(
    name = "SdfParser",
    src = "src/sta/sdf/SdfParse.yy",
    extra_outs = ["src/sta/sdf/SdfLocation.hh"],
    header_out = "src/sta/sdf/SdfParse.hh",
    prefix = "SdfParse_",
    source_out = "src/sta/sdf/SdfParse.cc",
)

genlex(
    name = "SaifLex",
    src = "src/sta/power/SaifLex.ll",
    out = "src/sta/power/SaifLex.cc",
    prefix = "SaifLex_",
)

genyacc(
    name = "SaifParser",
    src = "src/sta/power/SaifParse.yy",
    extra_outs = ["src/sta/power/SaifLocation.hh"],
    header_out = "src/sta/power/SaifParse.hh",
    prefix = "SaifParse_",
    source_out = "src/sta/power/SaifParse.cc",
)

# The order here is very important as the script to encode these relies on it
tcl_srcs = [
    "src/sta/tcl/Util.tcl",
    "src/sta/tcl/CmdUtil.tcl",
    "src/sta/tcl/Init.tcl",
    "src/sta/tcl/CmdArgs.tcl",
    "src/sta/tcl/Property.tcl",
    "src/sta/tcl/Sta.tcl",
    "src/sta/tcl/Variables.tcl",
    "src/sta/tcl/Splash.tcl",
    "src/sta/graph/Graph.tcl",
    "src/sta/liberty/Liberty.tcl",
    "src/sta/network/Link.tcl",
    "src/sta/network/Network.tcl",
    "src/sta/network/NetworkEdit.tcl",
    "src/sta/search/Search.tcl",
    "src/sta/dcalc/DelayCalc.tcl",
    "src/sta/parasitics/Parasitics.tcl",
    "src/sta/power/Power.tcl",
    "src/sta/sdc/Sdc.tcl",
    "src/sta/sdf/Sdf.tcl",
    "src/sta/verilog/Verilog.tcl",
]

exported_tcl = [
    "src/sta/tcl/Util.tcl",
    "src/sta/tcl/CmdUtil.tcl",
    "src/sta/tcl/CmdArgs.tcl",
    "src/sta/tcl/Property.tcl",
    "src/sta/tcl/Splash.tcl",
    "src/sta/tcl/Sta.tcl",
    "src/sta/tcl/Variables.tcl",
    "src/sta/sdc/Sdc.tcl",
    "src/sta/sdf/Sdf.tcl",
    "src/sta/search/Search.tcl",
    "src/sta/dcalc/DelayCalc.tcl",
    "src/sta/graph/Graph.tcl",
    "src/sta/liberty/Liberty.tcl",
    "src/sta/network/Network.tcl",
    "src/sta/network/NetworkEdit.tcl",
    "src/sta/parasitics/Parasitics.tcl",
    "src/sta/power/Power.tcl",
]

filegroup(
    name = "tcl_scripts",
    srcs = exported_tcl,
    visibility = [
        "//:__subpackages__",
    ],
)

tcl_encode_sta(
    name = "StaTclInitVar",
    srcs = tcl_srcs,
    char_array_name = "tcl_inits",
)

genrule(
    name = "StaConfig",
    srcs = [],
    outs = ["src/sta/util/StaConfig.hh"],
    cmd = """echo -e '
    #define STA_VERSION "2.2.1"
    #define STA_GIT_SHA1 "53d4d57cb8550d2ceed18adad75b73bba7858f4f"
    #define CUDD 0
    #define SSTA 0
    #define ZLIB_FOUND' > \"$@\"
    """,
)

filegroup(
    name = "sta_swig_files",
    srcs = [
        "src/sta/app/StaApp.i",
        "src/sta/dcalc/DelayCalc.i",
        "src/sta/graph/Graph.i",
        "src/sta/liberty/Liberty.i",
        "src/sta/network/Network.i",
        "src/sta/network/NetworkEdit.i",
        "src/sta/parasitics/Parasitics.i",
        "src/sta/power/Power.i",
        "src/sta/sdc/Sdc.i",
        "src/sta/sdf/Sdf.i",
        "src/sta/search/Search.i",
        "src/sta/spice/WriteSpice.i",
        "src/sta/tcl/Exception.i",
        "src/sta/tcl/StaTclTypes.i",
        "src/sta/util/Util.i",
        "src/sta/verilog/Verilog.i",
    ],
    visibility = ["//:__subpackages__"],
)

tcl_wrap_cc(
    name = "StaApp",
    srcs = [
        ":sta_swig_files",
    ],
    namespace_prefix = "sta",
    root_swig_src = "src/sta/app/StaApp.i",
    swig_includes = [
        "src/sta",
    ],
)

parser_cc = [
    # Liberty Expression Parser
    ":src/sta/liberty/LibExprParse.cc",
    ":src/sta/liberty/LibExprLex.cc",
    # Liberty Parser
    ":src/sta/liberty/LibertyLex.cc",
    ":src/sta/liberty/LibertyParse.cc",
    # Spef scan/parse.
    ":src/sta/parasitics/SpefLex.cc",
    ":src/sta/parasitics/SpefParse.cc",
    # Verilog scan/parse.
    ":src/sta/verilog/VerilogLex.cc",
    ":src/sta/verilog/VerilogParse.cc",
    # sdf scan/parse.
    ":src/sta/sdf/SdfLex.cc",
    ":src/sta/sdf/SdfParse.cc",
    # Saif scan/parse.
    ":src/sta/power/SaifLex.cc",
    ":src/sta/power/SaifParse.cc",
]

parser_headers = [
    # Liberty Expression Parser
    ":src/sta/liberty/LibExprParse.hh",
    # Liberty Parser
    ":src/sta/liberty/LibertyParse.hh",
    ":src/sta/liberty/LibertyLocation.hh",
    # Spef scan/parse.
    ":src/sta/parasitics/SpefParse.hh",
    ":src/sta/parasitics/SpefLocation.hh",
    # Verilog scan/parse.
    ":src/sta/verilog/VerilogParse.hh",
    ":src/sta/verilog/VerilogLocation.hh",
    # sdf scan/parse.
    ":src/sta/sdf/SdfParse.hh",
    ":src/sta/sdf/SdfLocation.hh",
    # Saif scan/parse.
    ":src/sta/power/SaifParse.hh",
    ":src/sta/power/SaifLocation.hh",
]

cc_binary(
    name = "opensta",
    srcs = [
        "src/sta/app/Main.cc",
        ":StaApp",
        ":StaTclInitVar",
    ],
    copts = [
        "-fexceptions",
        "-Wno-error",
        "-Wall",
        "-Wextra",
        "-pedantic",
        "-Wno-cast-qual",  # typically from TCL swigging
        "-Wno-missing-braces",  # typically from TCL swigging
        "-Wredundant-decls",
        "-Wformat-security",
        "-Wno-unused-parameter",
        "-Wno-sign-compare",
    ],
    features = ["-use_header_modules"],
    includes = [
        "src/sta/",
        "src/sta/dcalc",
        "src/sta/include/sta",
        "src/sta/util",
    ],
    malloc = "@tcmalloc//tcmalloc",
    visibility = ["//visibility:public"],
    deps = [
        ":opensta_lib",
        "@tk_tcl//:tcl",
    ],
)

cc_library(
    name = "opensta_lib",
    srcs = parser_cc + parser_headers + glob([
        "src/sta/dcalc/*.hh",
        "src/sta/util/*.hh",
        "src/sta/parasitics/*.hh",
        "src/sta/liberty/*.hh",
        "src/sta/sdc/*.hh",
        "src/sta/sdf/*.hh",
        "src/sta/search/*.hh",
        "src/sta/verilog/*.hh",
        "src/sta/power/*.hh",
        "src/sta/spice/*.hh",
    ]) + glob(
        include = [
            "src/sta/app/StaMain.cc",
            "src/sta/dcalc/*.cc",
            "src/sta/graph/*.cc",
            "src/sta/network/*.cc",
            "src/sta/util/*.cc",
            "src/sta/parasitics/*.cc",
            "src/sta/liberty/*.cc",
            "src/sta/sdc/*.cc",
            "src/sta/sdf/*.cc",
            "src/sta/search/*.cc",
            "src/sta/verilog/*.cc",
            "src/sta/power/*.cc",
            "src/sta/spice/*.cc",
            "src/sta/tcl/*.cc",
        ],
        exclude = [
            "src/sta/graph/Delay.cc",
            "src/sta/liberty/LibertyExt.cc",
            "src/sta/util/Machine*.cc",
        ],
    ) + [
        "src/sta/util/Machine.cc",
        ":StaConfig",
    ],
    #+ select({
    #        "@bazel_tools//src/conditions:windows": ["src/sta/util/MachineWin32.cc"],
    #        "@bazel_tools//src/conditions:darwin": ["src/sta/util/MachineApple.cc"],
    #        "@bazel_tools//src/conditions:linux": ["src/sta/util/MachineLinux.cc"],
    #        "//conditions:default": ["src/sta/util/MachineUnknown.cc"],
    #    })
    hdrs = glob(
        include = ["src/sta/include/sta/*.hh"],
    ),
    copts = [
        "-fexceptions",
        "-Wno-error",
        "-Wall",
        "-Wextra",
        "-pedantic",
        "-Wno-cast-qual",  # typically from TCL swigging
        "-Wno-missing-braces",  # typically from TCL swigging
        "-Wredundant-decls",
        "-Wformat-security",
        "-Wno-unused-parameter",
        "-Wno-sign-compare",
    ],
    features = [
        "-use_header_modules",
    ],
    includes = [
        "src/sta/",
        "src/sta/dcalc",
        "src/sta/include",
        "src/sta/include/sta",
        "src/sta/liberty",
        "src/sta/parasitics",
        "src/sta/power",
        "src/sta/sdf",
        "src/sta/util",
        "src/sta/verilog",
    ],
    textual_hdrs = ["src/sta/util/MachineLinux.cc"],
    visibility = ["//:__subpackages__"],
    deps = [
        "@cudd",
        "@eigen",
        "@rules_flex//flex:current_flex_toolchain",
        "@tk_tcl//:tcl",
        "@zlib",
    ],
)

filegroup(
    name = "tcl_util",
    srcs = [
        "src/sta/tcl/Util.tcl",
    ],
    visibility = ["//visibility:private"],
)

## OpenSTA
