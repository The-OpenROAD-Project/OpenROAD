# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# load("", "py_extension")
# load("", "py_library")
load("@bazel_skylib//rules:common_settings.bzl", "string_flag")

load("@rules_hdl//dependency_support/org_gnu_bison:bison.bzl", "genyacc")
load("@rules_hdl//dependency_support/com_github_westes_flex:flex.bzl", "genlex")

load(
    "//:build_helper.bzl",
    "OPENROAD_DEFINES",
    "OPENROAD_BINARY_DEPS",
    "OPENROAD_BINARY_SRCS",
    "OPENROAD_BINARY_SRCS_WITHOUT_MAIN",
    "OPENROAD_COPTS",
    "OPENROAD_LIBRARY_DEPS",
    "OPENROAD_LIBRARY_HDRS_INCLUDE",
    "OPENROAD_LIBRARY_INCLUDES",
    "OPENROAD_LIBRARY_SRCS_EXCLUDE",
    "OPENROAD_LIBRARY_SRCS_INCLUDE",
)
load("@rules_hdl//dependency_support/org_theopenroadproject:tcl_encode.bzl", "tcl_encode")

load("@//:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    features = [
        "-parse_headers",
        "-layering_check",
        # TODO(b/299593765): Fix strict ordering.
        "-libcxx_assertions",
    ],
)

# OpenRoad Physical Synthesis
licenses(["restricted"])

exports_files([
    "LICENSE",
    "src/sta/etc/TclEncode.tcl",
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

cc_binary(
    name = "openroad",
    srcs = OPENROAD_BINARY_SRCS + select({
        ":platform_cli": [],
    }),
    copts = OPENROAD_COPTS,
    features = ["-use_header_modules"],
    linkopts = select({
        ":platform_cli": [],
    }),
    visibility = ["//visibility:public"],
    deps = OPENROAD_BINARY_DEPS + [":openroad_lib_private"],
)

cc_library(
    name = "openroad_lib_private",
    srcs = glob(
        include = OPENROAD_LIBRARY_SRCS_INCLUDE + [
            #GUI Disabled
            "src/gui/src/stub.cpp",
            "src/gui/src/stub_heatMap.cpp",
        ],
        exclude = OPENROAD_LIBRARY_SRCS_EXCLUDE,
        allow_empty = True,
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
        "ENABLE_CHARTS=false",
        "ENABLE_mpl",
        "ENABLE_PAR",
    ],
    features = ["-use_header_modules"],
    includes = OPENROAD_LIBRARY_INCLUDES,
    deps = OPENROAD_LIBRARY_DEPS,
)

cc_library(
    name = "openroad_lib",
    srcs = OPENROAD_BINARY_SRCS_WITHOUT_MAIN + glob(
        include = OPENROAD_LIBRARY_SRCS_INCLUDE + [
            #GUI Disabled
            "src/gui/src/stub.cpp",
            "src/gui/src/stub_heatMap.cpp",
        ],
        exclude = OPENROAD_LIBRARY_SRCS_EXCLUDE,
        allow_empty = True,
    ) + [
        "src/Main_bindings.cc",
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
        "ENABLE_mpl",
        "ENABLE_PAR",
        "BUILD_GUI=false",
        "ENABLE_CHARTS=false",
    ],
    features = ["-use_header_modules"],
    includes = OPENROAD_LIBRARY_INCLUDES,
    visibility = ["//visibility:public"],
    deps = OPENROAD_LIBRARY_DEPS,
)

genrule(
    name = "post9",
    srcs = [
        "src/stt/src/flt/etc/MakeDatVar.tcl",
        "src/stt/src/flt/etc/POST9.dat",
    ],
    outs = ["src/stt/src/flt/etc/POST9.cpp"],
    cmd = "$(location @tk_tcl//:tclsh) $(location src/stt/src/flt/etc/MakeDatVar.tcl) post9 \"$@\" $(location src/stt/src/flt/etc/POST9.dat)",
    tools = ["@tk_tcl//:tclsh"],
)

genrule(
    name = "powv9",
    srcs = [
        "src/stt/src/flt/etc/MakeDatVar.tcl",
        "src/stt/src/flt/etc/POWV9.dat",
    ],
    outs = ["src/stt/src/flt/etc/POWV9.cpp"],
    cmd = "$(location @tk_tcl//:tclsh) $(location src/stt/src/flt/etc/MakeDatVar.tcl) powv9 \"$@\" $(location src/stt/src/flt/etc/POWV9.dat)",
    tools = ["@tk_tcl//:tclsh"],
)

cc_library(
    name = "logger",
    srcs = [
        "src/utl/src/CommandLineProgress.h",
        "src/utl/src/Logger.cpp",
        "src/utl/src/Metrics.cpp",
        "src/utl/src/ScopedTemporaryFile.cpp",
        "src/utl/src/prometheus/metrics_server.cpp",
    ],
    hdrs = [
        "src/utl/include/utl/Progress.h",
        "src/utl/include/utl/Logger.h",
        "src/utl/include/utl/Metrics.h",
        "src/utl/include/utl/ScopedTemporaryFile.h",
    ] + glob([
        "src/utl/include/utl/prometheus/*.h",
    ]),
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
        "src/utl/include",
        "src/utl/include/utl",
        "src/utl/src",
    ],
    visibility = ["@org_theopenroadproject//:__subpackages__"],
    deps = [
        "@com_github_gabime_spdlog//:spdlog",
    ],
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
    visibility = ["@org_theopenroadproject//:__subpackages__"],
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
        "src/odb/src/db/odb.tcl",
    ],
    out = "InitFloorplanTclInitVar.cc",
    char_array_name = "ifp_tcl_inits",
)

tcl_encode(
    name = "db_sta_tcl",
    srcs = [
        "src/dbSta/src/dbReadVerilog.tcl",
        "src/dbSta/src/dbSta.tcl",
        ":tcl_scripts",
    ],
    char_array_name = "dbSta_tcl_inits",
)

tcl_encode(
    name = "ioplacer_tcl",
    srcs = [
        "src/ppl/src/IOPlacer.tcl",
    ],
    char_array_name = "ppl_tcl_inits",
)

tcl_encode(
    name = "resizer_tcl",
    srcs = [
        "src/rsz/src/Resizer.tcl",
    ],
    char_array_name = "rsz_tcl_inits",
)

tcl_encode(
    name = "opendp_tcl",
    srcs = [
        "src/dpl/src/Opendp.tcl",
    ],
    char_array_name = "dpl_tcl_inits",
)

tcl_encode(
    name = "finale_tcl",
    srcs = [
        "src/fin/src/finale.tcl",
    ],
    char_array_name = "fin_tcl_inits",
)

tcl_encode(
    name = "ant_tcl",
    srcs = [
        "src/ant/src/AntennaChecker.tcl",
    ],
    char_array_name = "ant_tcl_inits",
)

tcl_encode(
    name = "fastroute_tcl",
    srcs = [
        "src/grt/src/GlobalRouter.tcl",
    ],
    char_array_name = "grt_tcl_inits",
)

tcl_encode(
    name = "tapcell_tcl",
    srcs = [
        "src/tap/src/tapcell.tcl",
    ],
    char_array_name = "tap_tcl_inits",
)

tcl_encode(
    name = "replace_tcl",
    srcs = [
        "src/gpl/src/replace.tcl",
    ],
    char_array_name = "gpl_tcl_inits",
)

tcl_encode(
    name = "triton_cts_tcl",
    srcs = [
        "src/cts/src/TritonCTS.tcl",
    ],
    char_array_name = "cts_tcl_inits",
)

tcl_encode(
    name = "rcx_tcl",
    srcs = [
        "src/rcx/src/OpenRCX.tcl",
    ],
    char_array_name = "rcx_tcl_inits",
)

tcl_encode(
    name = "triton_route_tcl",
    srcs = [
        "src/drt/src/TritonRoute.tcl",
    ],
    char_array_name = "drt_tcl_inits",
)

tcl_encode(
    name = "pdn_sim_tcl",
    srcs = [
        "src/psm/src/pdnsim.tcl",
    ],
    char_array_name = "psm_tcl_inits",
)

tcl_encode(
    name = "partition_manager_tcl",
    srcs = [
        "src/par/src/partitionmgr.tcl",
    ],
    char_array_name = "par_tcl_inits",
)

tcl_encode(
    name = "pdngen_tcl",
    srcs = [
        "src/pdn/src/pdn.tcl",
    ],
    char_array_name = "pdn_tcl_inits",
)

tcl_encode(
    name = "openroad_tcl",
    srcs = [":tcl_util"] + [
        "src/OpenRoad.tcl",
    ],
    char_array_name = "openroad_swig_tcl_inits",
)

tcl_encode(
    name = "pad_tcl",
    srcs = [
        "src/pad/src/pad.tcl",
    ],
    char_array_name = "pad_tcl_inits",
)

tcl_encode(
    name = "mpl_tcl",
    srcs = [
        "src/mpl/src/mpl.tcl",
    ],
    char_array_name = "mpl_tcl_inits",
)

tcl_encode(
    name = "rmp_tcl",
    srcs = [
        "src/rmp/src/rmp.tcl",
    ],
    char_array_name = "rmp_tcl_inits",
)

tcl_encode(
    name = "dst_tcl",
    srcs = [
        "src/dst/src/Distributed.tcl",
    ],
    char_array_name = "dst_tcl_inits",
)

tcl_encode(
    name = "stt_tcl",
    srcs = [
        "src/stt/src/SteinerTreeBuilder.tcl",
    ],
    char_array_name = "stt_tcl_inits",
)

tcl_encode(
    name = "dpo_tcl",
    srcs = [
        "src/dpo/src/Optdp.tcl",
    ],
    char_array_name = "dpo_tcl_inits",
)

tcl_encode(
    name = "gui_tcl",
    srcs = [
        "src/gui/src/gui.tcl",
    ],
    char_array_name = "gui_tcl_inits",
)

tcl_encode(
    name = "utl_tcl",
    srcs = [
        "src/utl/src/Utl.tcl",
    ],
    char_array_name = "utl_tcl_inits",
)

tcl_encode(
    name = "upf_tcl",
    srcs = [
        "src/upf/src/upf.tcl",
    ],
    char_array_name = "upf_tcl_inits",
)

tcl_encode(
    name = "dft_tcl",
    srcs = [
        "src/dft/src/dft.tcl",
    ],
    char_array_name = "dft_tcl_inits",
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
    ] + glob([
        "src/odb/src/swig/tcl/*.i",
        "src/odb/src/swig/common/*.i",
    ]),
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
        ":opendb_tcl",
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
    name = "logger_swig",
    srcs = [
        "src/utl/src/Logger.i",
        "src/utl/src/LoggerCommon.h",
        ":error_swig",
    ],
    module = "utl",
    namespace_prefix = "utl",
    root_swig_src = "src/utl/src/Logger.i",
    swig_includes = [
        "src/utl/src",
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
    ] + glob([
        "src/odb/src/swig/tcl/*.i",
        "src/odb/src/swig/common/*.i",
    ]),
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
    ] + glob([
        "src/odb/src/swig/tcl/*.i",
        "src/odb/src/swig/common/*.i",
        "src/odb/include/odb/*.h",
    ]),
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
    ] + glob([
        "src/odb/src/swig/tcl/*.i",
        "src/odb/src/swig/common/*.i",
        "src/odb/include/odb/*.h",
    ]),
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
    module = "Openroad_swig",
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
    ] + glob([
        "src/odb/src/swig/tcl/*.i",
        "src/odb/src/swig/common/*.i",
        "src/odb/include/odb/*.h",
    ]),
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

#tcl_wrap_cc(
#    name = "gui_swig",
#    srcs = [
#        "src/gui/src/gui.i",
#        ":error_swig",
#    ],
#    module = "gui",
#    namespace_prefix = "gui",
#    root_swig_src = "src/gui/src/gui.i",
#    runtime_header = "src/gui/src/tclSwig.h",
#    swig_includes = [
#        "src/gui/src",
#    ],
#)

filegroup(
    name = "error_swig",
    srcs = [
        "src/Exception.i",
    ],
)

filegroup(
    name = "design_swig",
    srcs = [
        "src/Design.i",
    ],
)

## OPENDB

filegroup(
    name = "opendb_tcl_common",
    srcs = [
        "src/odb/src/swig/common/swig_common.cpp",
        "src/odb/src/swig/common/swig_common.h",
    ],
    visibility = ["@org_theopenroadproject//:__subpackages__"],
)

tcl_wrap_cc(
    name = "opendb_tcl",
    srcs = glob([
        "src/odb/src/swig/tcl/*.i",
        "src/odb/src/swig/common/*.i",
        "src/odb/include/odb/*.h",
    ]) + [
        "src/Design.i",
        "src/Exception.i",
    ],
    module = "odbtcl",
    namespace_prefix = "odb",
    root_swig_src = "src/odb/src/swig/common/odb.i",
    swig_includes = [
        "src/odb/include",
        "src/odb/include/odb",
        "src/odb/src/swig/tcl",
    ],
    swig_options = [
        # These values are derived from the CMakeList.txt and represent the "rules" this swig file
        # breaks. Swig refuses to compile our swig files unless we acknowledge we are ignoring the
        # following warnings. They can be derived by attemtpting to compile without them and fixing
        # the warnings one by one.
        "-w509,503,501,472,467,402,401,317,325,378,383,389,365,362,314,258,240,203,201",
    ],
)

tcl_wrap_cc(
    name = "upf_swig",
    srcs = glob([
        "src/odb/include/odb/*.h",
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

cc_library(
    name = "opendb_lib",
    srcs = glob([
        "src/odb/src/cdl/*.cpp",
        "src/odb/src/db/*.cpp",
        "src/odb/src/db/*.h",
        "src/odb/src/db/*.hpp",
        "src/odb/src/zutil/*.cpp",
        "src/odb/src/zlib/*.cpp",
        "src/odb/src/defout/*.cpp",
        "src/odb/src/defout/*.h",
        "src/odb/src/defin/*.cpp",
        "src/odb/src/defin/*.h",
        "src/odb/src/lefin/*.cpp",
        "src/odb/src/lefin/*.h",
        "src/odb/src/lefout/*.cpp",
        "src/odb/src/lefout/*.h",
        "src/odb/src/tm/*.cpp",
    ]),
    hdrs = glob([
        "src/odb/include/odb/*.h",
        "src/odb/include/odb/*.hpp",
        "src/utl/include/utl/*.h",
        "src/odb/src/db/*.h",
    ]) + [
        "src/odb/src/def/def/defiAlias.hpp",
        "src/odb/src/def/def/defrReader.hpp",
        "src/odb/src/def/def/defwWriter.hpp",
        "src/odb/src/lef/lef/lefiDebug.hpp",
        "src/odb/src/lef/lef/lefiUtil.hpp",
        "src/odb/src/lef/lef/lefrReader.hpp",
        "src/odb/src/lef/lef/lefwWriter.hpp",
    ],
    copts = [
        "-fexceptions",
        "-Wno-error",
    ],
    features = [
        "-use_header_modules",
    ],
    includes = [
        "src/odb/include",
        "src/odb/include/odb",
        "src/odb/src/def/def",
        "src/odb/src/def/defzlib",
        "src/odb/src/def/lefzlib",
        "src/odb/src/lef/lef",
        "src/odb/src/lef/lefin",
        "src/odb/src/lef/lefzlib",
    ],
    visibility = ["@org_theopenroadproject//:__subpackages__"],
    deps = [
        ":logger",
        ":opendb_def",
        ":opendb_lef",
        "@boost//:algorithm",
        "@boost//:bind",
        "@boost//:config",
        "@boost//:fusion",
        "@boost//:geometry",
        "@boost//:lambda",
        "@boost//:optional",
        "@boost//:phoenix",
        "@boost//:polygon",
        "@boost//:property_tree",
        "@boost//:spirit",
        "@com_github_gabime_spdlog//:spdlog_with_exceptions",
        "@tk_tcl//:tcl",
        "@net_zlib//:zlib",
    ],
)

cc_library(
    name = "opendb_lef",
    srcs = glob(
        include = [
            "src/odb/src/lef/lef/*.cpp",
            "src/odb/src/lef/lef/*.h",
            "src/odb/src/lef/lef/*.hpp",
            "src/odb/src/lef/lefzlib/*.cpp",
        ],
        exclude = [
            "src/odb/src/lef/lef/lefiDebug.hpp",
            "src/odb/src/lef/lef/lefiUtil.hpp",
            "src/odb/src/lef/lef/lefrReader.hpp",
        ],
    ) + [
        "src/odb/src/lef/lef/lef_parser.cpp",
        "src/odb/src/lef/lef/lef_parser.hpp",
    ],
    hdrs = glob([
        "src/odb/include/odb/*.h",
        "src/odb/include/odb/*.hpp",
    ]) + [
        "src/odb/src/lef/lef/lefiDebug.hpp",
        "src/odb/src/lef/lef/lefiUtil.hpp",
        "src/odb/src/lef/lef/lefrReader.hpp",
        "src/odb/src/lef/lefzlib/lefzlib.hpp",
    ],
    copts = [
        "-fexceptions",
        "-Wno-error",
    ],
    features = ["-use_header_modules"],
    includes = [
        "src/odb/include/odb",
        "src/odb/src/lef/lef",
        "src/odb/src/lef/lefzlib",
    ],
    visibility = [
    	"//visibility:private",
    ],
    deps = [
        "@net_zlib//:zlib",
    ],
)

cc_library(
    name = "opendb_def",
    srcs = glob(
        include = [
            "src/odb/src/def/def/*.cpp",
            "src/odb/src/def/def/*.h",
            "src/odb/src/def/def/*.hpp",
            "src/odb/src/def/defzlib/*.hpp",
            "src/odb/src/def/defzlib/*.cpp",
        ],
        exclude = [
            "src/odb/src/def/def/defiComponent.hpp",
            "src/odb/src/def/def/defiUtil.hpp",
        ],
    ) + [
        "src/odb/src/def/def/def_parser.cpp",
        "src/odb/src/def/def/def_parser.hpp",
    ],
    hdrs = glob([
        "src/odb/include/odb/*.h",
        "src/odb/include/odb/*.hpp",
    ]) + [
        "src/odb/src/def/def/defiAlias.hpp",
        "src/odb/src/def/def/defiComponent.hpp",
        "src/odb/src/def/def/defiUtil.hpp",
        "src/odb/src/def/def/defrReader.hpp",
        "src/odb/src/def/def/defwWriter.hpp",
        "src/odb/src/def/defzlib/defzlib.hpp",
    ],
    copts = [
        "-fexceptions",
        "-Wno-error",
    ],
    features = ["-use_header_modules"],
    includes = [
        "src/odb/include/odb",
        "src/odb/src/def/def",
        "src/odb/src/def/defzlib",
    ],
    visibility = [
    	"//visibility:private",
    ],
    deps = [
        "@net_zlib//:zlib",
    ],
)

genyacc(
    name = "def_bison",
    src = "src/odb/src/def/def/def.y",
    header_out = "src/odb/src/def/def/def_parser.hpp",
    prefix = "defyy",
    source_out = "src/odb/src/def/def/def_parser.cpp",
)

genyacc(
    name = "lef_bison",
    src = "src/odb/src/lef/lef/lef.y",
    header_out = "src/odb/src/lef/lef/lef_parser.hpp",
    prefix = "lefyy",
    source_out = "src/odb/src/lef/lef/lef_parser.cpp",
)

## OPENDB

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
    	"@org_theopenroadproject//:__subpackages__",
    ],
)

tcl_encode(
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
    visibility = ["@org_theopenroadproject//:__subpackages__"],
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
    ]
    #+ select({
#        "@bazel_tools//src/conditions:windows": ["src/sta/util/MachineWin32.cc"],
#        "@bazel_tools//src/conditions:darwin": ["src/sta/util/MachineApple.cc"],
#        "@bazel_tools//src/conditions:linux": ["src/sta/util/MachineLinux.cc"],
#        "//conditions:default": ["src/sta/util/MachineUnknown.cc"],
#    })
,
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
    textual_hdrs = ["src/sta/util/MachineLinux.cc",],
    visibility = ["@org_theopenroadproject//:__subpackages__"],
    deps = [
        "@tk_tcl//:tcl",
        "@net_zlib//:zlib",
        "@eigen//:eigen3",
        "@com_github_ivmai_cudd//:cudd",
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

