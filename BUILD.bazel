# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

load("@bazel_skylib//rules:common_settings.bzl", "string_flag")
load("@rules_hdl//dependency_support/com_github_westes_flex:flex.bzl", "genlex")
load("@rules_hdl//dependency_support/org_gnu_bison:bison.bzl", "genyacc")
load(
    "//:bazel/build_helper.bzl",
    "OPENROAD_BINARY_SRCS_WITHOUT_MAIN",
    "OPENROAD_LIBRARY_HDRS_INCLUDE",
    "OPENROAD_LIBRARY_INCLUDES",
    "OPENROAD_LIBRARY_SRCS_EXCLUDE",
    "OPENROAD_LIBRARY_SRCS_INCLUDE",
)
load("//:bazel/tcl_encode_sta.bzl", "tcl_encode_sta")
load("//:bazel/tcl_encode_or.bzl", "tcl_encode")
load("//:bazel/tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    features = [
        "-parse_headers",
        "-layering_check",
        # TODO(b/299593765): Fix strict ordering.
        "-libcxx_assertions",
    ],
)

exports_files([
    "LICENSE",
    "src/sta/etc/TclEncode.tcl",
    "src/Design.i",
    "src/Exception.i",
])

string_flag(
    name = "platform",
    build_setting_default = "cli",
    values = [
        "cli",
    ],
)

config_setting(
    name = "platform_cli",
    flag_values = {
        ":platform": "cli",
    },
)

# TODO: once project is properly decomposed, we don't
# need these blanked dependencies in multiple places anymore.
OPENROAD_LIBRARY_DEPS = [
    "//src/utl",
    ":munkres",
    "//src/odb",
    "//src/gui",
    ":openroad_version",
    ":opensta_lib",
    "@boost.asio",
    "@boost.geometry",
    "@boost.graph",
    "@boost.heap",
    "@boost.icl",
    "@boost.json",
    "@boost.multi_array",
    "@boost.polygon",
    "@boost.property_tree",
    "@boost.stacktrace",
    "@boost.thread",
    "@com_github_quantamhd_lemon//:lemon",
    "@edu_berkeley_abc//:abc-lib",
    "@eigen",
    "@or-tools//ortools/base:base",
    "@or-tools//ortools/linear_solver:linear_solver",
    "@or-tools//ortools/linear_solver:linear_solver_cc_proto",
    "@or-tools//ortools/sat:cp_model",
    "@org_llvm_openmp//:openmp",
    "@spdlog",
    "@tk_tcl//:tcl",
]

OPENROAD_COPTS = [
    "-fexceptions",
    "-ffp-contract=off",  # Needed for floating point stability.
    "-Wno-error",
    "-Wall",
    "-Wextra",
    "-pedantic",
    "-Wno-cast-qual",  # typically from TCL swigging
    "-Wno-missing-braces",  # typically from TCL swigging
    "-Wredundant-decls",
    "-Wformat-security",
    "-Wno-sign-compare",
    "-Wno-unused-parameter",
]

OPENROAD_DEFINES = [
    "OPENROAD_GIT_DESCRIBE=\\\"bazel-build\\\"",
    "BUILD_TYPE=\\\"$(COMPILATION_MODE)\\\"",
    "GPU=false",
    "BUILD_PYTHON=false",
    "ABC_NAMESPACE=abc",
    "TCLRL_VERSION_STR=",
]

cc_binary(
    name = "openroad",
    srcs = OPENROAD_BINARY_SRCS_WITHOUT_MAIN + [
        "src/Main.cc",
        "src/OpenRoad.cc",
    ] + select({
        ":platform_cli": [],
    }),
    copts = OPENROAD_COPTS,
    features = ["-use_header_modules"],
    linkopts = select({
        ":platform_cli": [],
    }),
    malloc = "@tcmalloc//tcmalloc",
    visibility = ["//visibility:public"],
    deps = [
        ":openroad_lib_private",
        ":openroad_version",
        ":opensta_lib",
        "//src/odb",
        "//src/utl",
        "@rules_cc//cc/runfiles",
        "@tk_tcl//:tcl",
    ],
)

cc_library(
    name = "openroad_lib_private",
    srcs = glob(
        include = OPENROAD_LIBRARY_SRCS_INCLUDE,
        allow_empty = True,
        exclude = OPENROAD_LIBRARY_SRCS_EXCLUDE,
    ) + [
        "src/stt/src/flt/etc/POST9.cpp",
        "src/stt/src/flt/etc/POWV9.cpp",
    ],
    hdrs = glob(
        include = OPENROAD_LIBRARY_HDRS_INCLUDE,
        exclude = [
            "src/utl/include/utl/Logger.h",
            "src/utl/include/utl/CFileUtils.h",
        ],
    ),
    copts = OPENROAD_COPTS,
    defines = OPENROAD_DEFINES + [
        "BUILD_GUI=false",
    ],
    features = ["-use_header_modules"],
    includes = OPENROAD_LIBRARY_INCLUDES,
    deps = OPENROAD_LIBRARY_DEPS,
)

cc_library(
    name = "openroad_lib",
    srcs = OPENROAD_BINARY_SRCS_WITHOUT_MAIN + glob(
        include = OPENROAD_LIBRARY_SRCS_INCLUDE,
        allow_empty = True,
        exclude = OPENROAD_LIBRARY_SRCS_EXCLUDE,
    ) + [
        "src/OpenRoad.cc",
        "src/stt/src/flt/etc/POST9.cpp",
        "src/stt/src/flt/etc/POWV9.cpp",
    ],
    hdrs = glob(
        include = OPENROAD_LIBRARY_HDRS_INCLUDE,
        exclude = [
            "src/utl/include/utl/Logger.h",
            "src/utl/include/utl/CFileUtils.h",
        ],
    ),
    copts = OPENROAD_COPTS,
    defines = OPENROAD_DEFINES + [
        "BUILD_GUI=false",
    ],
    features = ["-use_header_modules"],
    includes = OPENROAD_LIBRARY_INCLUDES,
    visibility = ["//visibility:public"],
    deps = OPENROAD_LIBRARY_DEPS,
)

genrule(
    name = "post9",
    srcs = [
        "etc/file_to_string.py",
        "src/stt/src/flt/etc/POST9.dat",
    ],
    outs = ["src/stt/src/flt/etc/POST9.cpp"],
    cmd = "$(location etc/file_to_string.py) --inputs $(location src/stt/src/flt/etc/POST9.dat) --output \"$@\" --varname post9 --namespace stt::flt",
)

genrule(
    name = "powv9",
    srcs = [
        "etc/file_to_string.py",
        "src/stt/src/flt/etc/POWV9.dat",
    ],
    outs = ["src/stt/src/flt/etc/POWV9.cpp"],
    cmd = "$(location etc/file_to_string.py) --inputs $(location src/stt/src/flt/etc/POWV9.dat) --output \"$@\" --varname powv9 --namespace stt::flt",
)

cc_library(
    name = "munkres",
    srcs = glob([
        "src/ppl/src/munkres/src/*.cpp",
    ]),
    hdrs = glob([
        "src/ppl/src/munkres/src/*.h",
    ]),
    includes = [
        "src/ppl/src/munkres/src",
    ],
    visibility = ["//:__subpackages__"],
)

cc_library(
    name = "openroad_version",
    hdrs = [
        ":OpenRoadVersion",
    ],
    includes = [
        "include/ord",
    ],
)

genrule(
    name = "OpenRoadVersion",
    srcs = [],
    outs = ["include/ord/Version.hh"],
    cmd = """echo -e '
#define OPENROAD_VERSION "HDL-HEAD"
#define OPENROAD_GIT_SHA1 "HEAD"
' > \"$@\"
    """,
)

tcl_encode(
    name = "init_floorplan_tcl",
    srcs = [
        "src/ifp/src/InitFloorplan.tcl",
    ],
    out = "InitFloorplanTclInitVar.cc",
    char_array_name = "ifp_tcl_inits",
    namespace = "ifp",
)

tcl_encode(
    name = "db_sta_tcl",
    srcs = [
        "src/dbSta/src/dbReadVerilog.tcl",
        "src/dbSta/src/dbSta.tcl",
        ":tcl_scripts",
    ],
    char_array_name = "dbSta_tcl_inits",
    namespace = "sta",
)

tcl_encode(
    name = "ioplacer_tcl",
    srcs = [
        "src/ppl/src/IOPlacer.tcl",
    ],
    char_array_name = "ppl_tcl_inits",
    namespace = "ppl",
)

tcl_encode(
    name = "resizer_tcl",
    srcs = [
        "src/rsz/src/Resizer.tcl",
    ],
    char_array_name = "rsz_tcl_inits",
    namespace = "rsz",
)

tcl_encode(
    name = "opendp_tcl",
    srcs = [
        "src/dpl/src/Opendp.tcl",
    ],
    char_array_name = "dpl_tcl_inits",
    namespace = "dpl",
)

tcl_encode(
    name = "finale_tcl",
    srcs = [
        "src/fin/src/finale.tcl",
    ],
    char_array_name = "fin_tcl_inits",
    namespace = "fin",
)

tcl_encode(
    name = "ant_tcl",
    srcs = [
        "src/ant/src/AntennaChecker.tcl",
    ],
    char_array_name = "ant_tcl_inits",
    namespace = "ant",
)

tcl_encode(
    name = "fastroute_tcl",
    srcs = [
        "src/grt/src/GlobalRouter.tcl",
    ],
    char_array_name = "grt_tcl_inits",
    namespace = "grt",
)

tcl_encode(
    name = "tapcell_tcl",
    srcs = [
        "src/tap/src/tapcell.tcl",
    ],
    char_array_name = "tap_tcl_inits",
    namespace = "tap",
)

tcl_encode(
    name = "replace_tcl",
    srcs = [
        "src/gpl/src/replace.tcl",
    ],
    char_array_name = "gpl_tcl_inits",
    namespace = "gpl",
)

tcl_encode(
    name = "triton_cts_tcl",
    srcs = [
        "src/cts/src/TritonCTS.tcl",
    ],
    char_array_name = "cts_tcl_inits",
    namespace = "cts",
)

tcl_encode(
    name = "rcx_tcl",
    srcs = [
        "src/rcx/src/OpenRCX.tcl",
    ],
    char_array_name = "rcx_tcl_inits",
    namespace = "rcx",
)

tcl_encode(
    name = "triton_route_tcl",
    srcs = [
        "src/drt/src/TritonRoute.tcl",
    ],
    char_array_name = "drt_tcl_inits",
    namespace = "drt",
)

tcl_encode(
    name = "pdn_sim_tcl",
    srcs = [
        "src/psm/src/pdnsim.tcl",
    ],
    char_array_name = "psm_tcl_inits",
    namespace = "psm",
)

tcl_encode(
    name = "partition_manager_tcl",
    srcs = [
        "src/par/src/partitionmgr.tcl",
    ],
    char_array_name = "par_tcl_inits",
    namespace = "par",
)

tcl_encode(
    name = "pdngen_tcl",
    srcs = [
        "src/pdn/src/pdn.tcl",
    ],
    char_array_name = "pdn_tcl_inits",
    namespace = "pdn",
)

tcl_encode(
    name = "openroad_tcl",
    srcs = [":tcl_util"] + [
        "src/Metrics.tcl",
        "src/OpenRoad.tcl",
    ],
    char_array_name = "ord_tcl_inits",
    namespace = "ord",
)

tcl_encode(
    name = "pad_tcl",
    srcs = [
        "src/pad/src/pad.tcl",
    ],
    char_array_name = "pad_tcl_inits",
    namespace = "pad",
)

tcl_encode(
    name = "mpl_tcl",
    srcs = [
        "src/mpl/src/mpl.tcl",
    ],
    char_array_name = "mpl_tcl_inits",
    namespace = "mpl",
)

tcl_encode(
    name = "rmp_tcl",
    srcs = [
        "src/rmp/src/rmp.tcl",
    ],
    char_array_name = "rmp_tcl_inits",
    namespace = "rmp",
)

tcl_encode(
    name = "dst_tcl",
    srcs = [
        "src/dst/src/Distributed.tcl",
    ],
    char_array_name = "dst_tcl_inits",
    namespace = "dst",
)

tcl_encode(
    name = "stt_tcl",
    srcs = [
        "src/stt/src/SteinerTreeBuilder.tcl",
    ],
    char_array_name = "stt_tcl_inits",
    namespace = "stt",
)

tcl_encode(
    name = "dpo_tcl",
    srcs = [
        "src/dpo/src/Optdp.tcl",
    ],
    char_array_name = "dpo_tcl_inits",
    namespace = "dpo",
)

tcl_encode(
    name = "upf_tcl",
    srcs = [
        "src/upf/src/upf.tcl",
    ],
    char_array_name = "upf_tcl_inits",
    namespace = "upf",
)

tcl_encode(
    name = "dft_tcl",
    srcs = [
        "src/dft/src/dft.tcl",
    ],
    char_array_name = "dft_tcl_inits",
    namespace = "dft",
)

tcl_wrap_cc(
    name = "dst_swig",
    srcs = [
        "src/dst/src/Distributed.i",
        ":error_swig",
    ],
    module = "dst",
    namespace_prefix = "dst",
    root_swig_src = "src/dst/src/Distributed.i",
    swig_includes = [
        "src/dst/src",
    ],
)

tcl_wrap_cc(
    name = "init_floorplan_swig",
    srcs = [
        "src/ifp/src/InitFloorplan.i",
        ":design_swig",
        ":error_swig",
        "//src/odb:swig_imports",
    ],
    module = "ifp",
    namespace_prefix = "ifp",
    root_swig_src = "src/ifp/src/InitFloorplan.i",
    swig_includes = [
        "src/ifp/src",
        "src/odb/src/swig/common",
        "src/odb/src/swig/tcl",
    ],
)

tcl_wrap_cc(
    name = "dbsta_swig",
    srcs = [
        "src/dbSta/src/dbSta.i",
        ":error_swig",
        ":sta_swig_files",
    ],
    module = "dbSta",
    namespace_prefix = "sta",
    root_swig_src = "src/dbSta/src/dbSta.i",
    swig_includes = [
        "src/dbSta/src",
        "src/odb/src/swig/common",
        "src/sta",
    ],
    deps = [
        "//src/odb:swig",
    ],
)

tcl_wrap_cc(
    name = "ioplacer_swig",
    srcs = [
        "src/ppl/src/IOPlacer.i",
        ":error_swig",
    ],
    module = "ppl",
    namespace_prefix = "ppl",
    root_swig_src = "src/ppl/src/IOPlacer.i",
    swig_includes = [
        "src/ppl/src",
    ],
)

tcl_wrap_cc(
    name = "resizer_swig",
    srcs = [
        "src/rsz/src/Resizer.i",
        ":error_swig",
        ":sta_swig_files",
    ],
    module = "rsz",
    namespace_prefix = "rsz",
    root_swig_src = "src/rsz/src/Resizer.i",
    swig_includes = [
        "src/rsz/src",
        "src/sta",
    ],
)

tcl_wrap_cc(
    name = "opendp_swig",
    srcs = [
        "src/dpl/src/Opendp.i",
        ":error_swig",
    ],
    module = "dpl",
    namespace_prefix = "dpl",
    root_swig_src = "src/dpl/src/Opendp.i",
    swig_includes = [
        "src/dpl/src",
    ],
)

tcl_wrap_cc(
    name = "finale_swig",
    srcs = [
        "src/fin/src/finale.i",
        ":error_swig",
    ],
    module = "fin",
    namespace_prefix = "fin",
    root_swig_src = "src/fin/src/finale.i",
    swig_includes = [
        "src/fin/src",
    ],
)

tcl_wrap_cc(
    name = "ant_swig",
    srcs = [
        "src/ant/src/AntennaChecker.i",
        ":error_swig",
    ],
    module = "ant",
    namespace_prefix = "ant",
    root_swig_src = "src/ant/src/AntennaChecker.i",
    swig_includes = [
        "src/ant/src",
    ],
)

tcl_wrap_cc(
    name = "fastroute_swig",
    srcs = [
        "src/grt/src/GlobalRouter.i",
        ":error_swig",
    ],
    module = "grt",
    namespace_prefix = "grt",
    root_swig_src = "src/grt/src/GlobalRouter.i",
    swig_includes = [
        "src/grt/src",
    ],
)

tcl_wrap_cc(
    name = "replace_swig",
    srcs = [
        "src/gpl/src/replace.i",
        ":error_swig",
        "//src/odb:swig_imports",
    ],
    module = "gpl",
    namespace_prefix = "gpl",
    root_swig_src = "src/gpl/src/replace.i",
    swig_includes = [
        "src/gpl/src",
        "src/odb/src/swig/common",
        "src/odb/src/swig/tcl",
    ],
)

tcl_wrap_cc(
    name = "triton_cts_swig",
    srcs = [
        "src/cts/src/TritonCTS.i",
        ":error_swig",
    ],
    module = "cts",
    namespace_prefix = "cts",
    root_swig_src = "src/cts/src/TritonCTS.i",
    swig_includes = [
        "src/cts/src",
    ],
)

tcl_wrap_cc(
    name = "tapcell_swig",
    srcs = [
        "src/tap/src/tapcell.i",
        ":error_swig",
        "//src/odb:swig_imports",
    ],
    module = "tap",
    namespace_prefix = "tap",
    root_swig_src = "src/tap/src/tapcell.i",
    swig_includes = [
        "src/odb/src/swig/common",
        "src/odb/src/swig/tcl",
        "src/tap/src",
    ],
)

tcl_wrap_cc(
    name = "rcx_swig",
    srcs = [
        "src/rcx/src/ext.i",
        ":design_swig",
        ":error_swig",
    ],
    module = "rcx",
    namespace_prefix = "rcx",
    root_swig_src = "src/rcx/src/ext.i",
    swig_includes = [
        "src/rcx/src",
    ],
)

tcl_wrap_cc(
    name = "triton_route_swig",
    srcs = [
        "src/drt/src/TritonRoute.i",
        ":error_swig",
    ],
    module = "drt",
    namespace_prefix = "drt",
    root_swig_src = "src/drt/src/TritonRoute.i",
    swig_includes = [
        "src/drt/src",
    ],
)

tcl_wrap_cc(
    name = "pdn_sim_swig",
    srcs = [
        "src/psm/src/pdnsim.i",
        ":error_swig",
    ],
    module = "psm",
    namespace_prefix = "psm",
    root_swig_src = "src/psm/src/pdnsim.i",
    swig_includes = [
        "src/psm/src",
    ],
)

tcl_wrap_cc(
    name = "partition_manager_swig",
    srcs = [
        "src/par/src/partitionmgr.i",
        ":error_swig",
    ],
    module = "Par",
    namespace_prefix = "par",
    root_swig_src = "src/par/src/partitionmgr.i",
    swig_includes = [
        "src/par/src",
    ],
)

tcl_wrap_cc(
    name = "pdngen_swig",
    srcs = [
        "src/pdn/src/PdnGen.i",
        ":error_swig",
        "//src/odb:swig_imports",
    ],
    module = "pdn",
    namespace_prefix = "pdn",
    root_swig_src = "src/pdn/src/PdnGen.i",
    swig_includes = [
        "src/odb/src/swig/common",
        "src/odb/src/swig/tcl",
        "src/pdn/src",
    ],
)

tcl_wrap_cc(
    name = "openroad_swig",
    srcs = [
        "src/OpenRoad.i",
        ":error_swig",
    ],
    module = "ord",
    namespace_prefix = "ord",
    root_swig_src = "src/OpenRoad.i",
    swig_includes = [
        "src",
    ],
)

tcl_wrap_cc(
    name = "pad_swig",
    srcs = [
        "src/pad/src/pad.i",
        ":error_swig",
        "//src/odb:swig_imports",
    ],
    module = "pad",
    namespace_prefix = "pad",
    root_swig_src = "src/pad/src/pad.i",
    swig_includes = [
        "src/odb/include",
        "src/odb/src/swig/common",
        "src/odb/src/swig/tcl",
        "src/pad/src",
    ],
)

tcl_wrap_cc(
    name = "mpl_swig",
    srcs = [
        "src/mpl/src/mpl.i",
        ":error_swig",
    ],
    module = "mpl",
    namespace_prefix = "mpl",
    root_swig_src = "src/mpl/src/mpl.i",
    swig_includes = [
        "src/mpl/src",
    ],
)

tcl_wrap_cc(
    name = "rmp_swig",
    srcs = [
        "src/rmp/src/rmp.i",
        ":error_swig",
    ],
    module = "rmp",
    namespace_prefix = "rmp",
    root_swig_src = "src/rmp/src/rmp.i",
    swig_includes = [
        "src/rmp/src",
    ],
)

tcl_wrap_cc(
    name = "stt_swig",
    srcs = [
        "src/stt/src/SteinerTreeBuilder.i",
        ":error_swig",
    ],
    module = "stt",
    namespace_prefix = "stt",
    root_swig_src = "src/stt/src/SteinerTreeBuilder.i",
    swig_includes = [
        "src/stt/src",
    ],
)

tcl_wrap_cc(
    name = "dpo_swig",
    srcs = [
        "src/dpo/src/Optdp.i",
        ":error_swig",
    ],
    module = "dpo",
    namespace_prefix = "dpo",
    root_swig_src = "src/dpo/src/Optdp.i",
    swig_includes = [
        "src/dpo/src",
    ],
)

tcl_wrap_cc(
    name = "dft_swig",
    srcs = [
        "src/dft/src/dft.i",
        ":error_swig",
    ],
    module = "dft",
    namespace_prefix = "dft",
    root_swig_src = "src/dft/src/dft.i",
    swig_includes = [
        "src/dft/src",
    ],
)

filegroup(
    name = "error_swig",
    srcs = [
        "src/Exception.i",
    ],
    visibility = ["@//:__subpackages__"],
)

filegroup(
    name = "design_swig",
    srcs = [
        "src/Design.i",
    ],
)

tcl_wrap_cc(
    name = "upf_swig",
    srcs = glob([
        "src/upf/include/upf/*.h",
    ]) + [
        "src/upf/src/upf.i",
    ],
    module = "upf",
    namespace_prefix = "upf",
    root_swig_src = "src/upf/src/upf.i",
    swig_includes = [
        "src/odb/include",
        "src/upf/include",
    ],
)

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
