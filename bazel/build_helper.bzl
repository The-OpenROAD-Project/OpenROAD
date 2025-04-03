###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2025, Precision Innovations Inc.
## Copyright (c) 2025 Google LLC
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

"""Source Tracking for OpenROAD"""

OPENROAD_BINARY_SRCS_WITHOUT_MAIN = [
    #Root OpenRoad
    ":openroad_swig",
    ":openroad_tcl",
    #Utility
    "//src/utl:utl_swig",
    "//src/utl:utl_tcl",
    #InitFp
    ":init_floorplan_swig",
    ":init_floorplan_tcl",
    #OpenDB
    ":opendb_tcl",
    ":opendb_tcl_common",
    ":upf_swig",
    ":upf_tcl",
    #DbSTA
    ":db_sta_tcl",
    ":dbsta_swig",
    #ioPlacer
    ":ioplacer_tcl",
    ":ioplacer_swig",
    #Resizer
    ":resizer_swig",
    ":resizer_tcl",
    #OpenDP
    ":opendp_swig",
    ":opendp_tcl",
    #finale
    ":finale_swig",
    ":finale_tcl",
    #antenna_checker
    ":ant_swig",
    ":ant_tcl",
    #FastRoute
    ":fastroute_swig",
    ":fastroute_tcl",
    #Replace
    ":replace_swig",
    ":replace_tcl",
    #TritonCTS
    ":triton_cts_tcl",
    ":triton_cts_swig",
    #Tapcell
    ":tapcell_swig",
    ":tapcell_tcl",
    #OpenRCX
    ":rcx_swig",
    ":rcx_tcl",
    #TritonRoute
    ":triton_route_swig",
    ":triton_route_tcl",
    #PDNSim
    ":pdn_sim_swig",
    ":pdn_sim_tcl",
    #PartitionManager
    ":partition_manager_swig",
    ":partition_manager_tcl",
    #PDNGen
    ":pdngen_tcl",
    ":pdngen_swig",
    #MPL
    ":mpl_swig",
    ":mpl_tcl",
    #RMP
    ":rmp_swig",
    ":rmp_tcl",
    #STT
    ":stt_swig",
    ":stt_tcl",
    #Distributed
    ":dst_swig",
    ":dst_tcl",
    #Dpo
    ":dpo_swig",
    ":dpo_tcl",
    #Pad
    ":pad_swig",
    ":pad_tcl",
    #dft
    ":dft_swig",
    ":dft_tcl",
]

OPENROAD_BINARY_SRCS = OPENROAD_BINARY_SRCS_WITHOUT_MAIN + [
    #Root OpenRoad
    "src/Main.cc",
    "src/OpenRoad.cc",
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
    "BUILD_TYPE=\\\"release\\\"",
    "GPU=false",
    "BUILD_PYTHON=false",
    "ABC_NAMESPACE=abc",
    "TCLRL_VERSION_STR=",
]

OPENROAD_BINARY_DEPS = [
    ":opendb_lib",
    ":openroad_version",
    ":opensta_lib",
    "@tk_tcl//:tcl",
]

OPENROAD_LIBRARY_HDRS_INCLUDE = [
    #Root OpenRoad
    "include/ord/*.h",
    "include/ord/*.hh",
    #Utility
    "src/utl/include/utl/*.h",
    #InitFp
    "src/ifp/include/ifp/*.hh",
    #GUI
    "src/gui/include/gui/*.h",
    #STA
    "src/sta/include/sta/*.hh",
    #DbSTA
    "src/dbSta/include/db_sta/*.hh",
    #ioPlacer
    "src/ppl/include/ppl/*.h",
    #Resizer
    "src/rsz/include/rsz/*.hh",
    #OpenDP
    "src/dpl/include/dpl/*.h",
    #finale
    "src/fin/include/fin/*.h",
    #TritonMP
    "src/mpl/include/mpl/*.h",
    #antenna_checker
    "src/ant/include/ant/*.hh",
    "src/ant/src/*.hh",
    #FastRoute
    "src/grt/src/fastroute/include/*.h",
    "src/grt/include/grt/*.h",
    #Replace
    "src/gpl/include/gpl/*.h",
    #TritonCTS
    "src/cts/include/cts/*.h",
    #Tapcell
    "src/tap/include/tap/*.h",
    #OpenRCX
    "src/rcx/include/rcx/*.h",
    #TritonRoute
    "src/drt/include/triton_route/*.h",
    "src/drt/src/db/infra/*.hpp",
    #PDNSim
    "src/psm/include/psm/*.h",
    "src/psm/include/psm/*.hh",
    #PartitionManager
    "src/par/src/*.h",
    "src/par/include/par/*.h",
    #PDNGen
    "src/pdn/include/pdn/*.hh",
    #STT
    "src/stt/include/stt/*.h",
    #MPL
    "src/mpl/include/mpl/*.h",
    #RMP
    "src/rmp/src/*.h",
    "src/rmp/include/rmp/*.h",
    #Distributed
    "src/dst/include/dst/*.h",
    #Dpo
    "src/dpo/include/dpo/*.h",
    #pad
    "src/pad/include/pad/*.h",
    #dft
    "src/dft/include/dft/*.hh",
    #upf
    "src/upf/include/upf/*.h",
    "src/upf/src/*.h",
]

OPENROAD_LIBRARY_INCLUDES = [
    #Root OpenRoad
    "include",
    #OpenDB
    "src/odb/include/odb",
    #OpenDBTCL
    "src/odb/src/swig/common",
    #STA
    "src/sta/include/sta",
    #DbSTA
    "src/dbSta/include",
    "src/dbSta/include/db_sta",
    #GUI
    "src/gui/include",
    #InitFp
    "src/ifp/include",
    #ioPlacer
    "src/ppl/include",
    "src/ppl/include/ppl",
    "src/ppl/src",
    #Resizer
    "src/rsz/include",
    "src/rsz/include/rsz",
    "src/rsz/src",
    #OpenDP
    "src/dpl/src",
    "src/dpl/include",
    "src/dpl/include/dpl",
    #finale
    "src/fin/include",
    "src/fin/include/fin",
    #TritonMP
    "src/mpl/include",
    "src/mpl/include/mpl",
    #antenna_checker
    "src/ant/include",
    "src/ant/include/ant",
    #FastRoute
    "src/grt/src/fastroute/include",
    "src/grt/include/grt",
    "src/grt/include",
    "src/grt/src",
    #Replace
    "src/gpl/include/gpl",
    "src/gpl/include",
    #TritonCTS
    "src/cts/src",
    "src/cts/include",
    #Tapcell
    "src/tap/include/tap",
    "src/tap/include",
    #OpenRCX
    "src/rcx/include/rcx",
    "src/rcx/include",
    #TritonRoute
    "src/drt/include/triton_route",
    "src/drt/src",
    "src/drt/include",
    #PDNSim
    "src/psm/include/psm",
    "src/psm/include",
    #PartitionManager
    "src/par/include",
    "src/par/include/par",
    #PDNGen
    "src/pdn/include",
    "src/pdn/include/pdn",
    #MPL
    "src/mpl/include",
    "src/mpl/src",
    #RMP
    "src/rmp/include",
    #STT
    "src/stt/include",
    "src/stt/include/stt",
    "src/stt/src",
    #Distributed
    "src/dst/include",
    "src/dst/include/dst",
    #Dpo
    "src/dpo/include",
    "src/dpo/include/dpo",
    #pad
    "src/pad/include",
    #utl
    "src/utl/src",
    #dft
    "src/dft/include",
    "src/dft/src/clock_domain",
    "src/dft/src/config",
    "src/dft/src/utils",
    "src/dft/src/cells",
    "src/dft/src/replace",
    "src/dft/src/architect",
    "src/dft/src/stitch",
    #upf
    "src/upf/include",
]

OPENROAD_LIBRARY_DEPS = [
    ":munkres",
    ":opendb_lib",
    ":openroad_version",
    ":opensta_lib",
    "@or-tools//ortools/base:base",
    "@or-tools//ortools/linear_solver:linear_solver",
    "@or-tools//ortools/linear_solver:linear_solver_cc_proto",
    "@or-tools//ortools/sat:cp_model",
    "@edu_berkeley_abc//:abc-lib",
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
    "@eigen",
    "@com_github_quantamhd_lemon//:lemon",
    "@org_llvm_openmp//:openmp",
    "@spdlog",
    "@tk_tcl//:tcl",
]

OPENROAD_LIBRARY_SRCS_EXCLUDE = [
    "src/Main.cc",
    "src/OpenRoad.cc",
]

OPENROAD_LIBRARY_SRCS_INCLUDE = [
    #Root OpenRoad
    "src/*.cc",
    "src/*.cpp",
    #Utility
    "src/utl/src/*.cpp",
    "src/utl/src/*.h",
    #InitFp
    "src/ifp/src/*.cc",
    #DbSTA
    "src/dbSta/src/*.cc",
    "src/dbSta/src/*.cpp",
    "src/dbSta/src/*.hh",
    "src/dbSta/src/*.h",
    #ioPlacer
    "src/ppl/src/*.cpp",
    "src/ppl/src/*.h",
    #Resizer
    "src/rsz/src/*.cc",
    "src/rsz/src/*.hh",
    "src/rsz/src/*.h",
    #OpenDP
    "src/dpl/src/*.cpp",
    "src/dpl/src/*.h",
    #finale
    "src/fin/src/*.cpp",
    "src/fin/src/*.h",
    #TritionMP
    "src/mpl/src/*.cpp",
    "src/mpl/src/*.h",
    #antenna_checker
    "src/ant/src/*.cc",
    #FastRoute
    "src/grt/src/fastroute/src/*.cpp",
    "src/grt/src/fastroute/src/*.h",
    "src/grt/src/*.h",
    "src/grt/src/*.cpp",
    #Replace
    "src/gpl/src/*.cpp",
    "src/gpl/src/*.h",
    #TritonCTS
    "src/cts/src/*.h",
    "src/cts/src/*.cpp",
    #Tapcell
    "src/tap/src/*.cpp",
    #OpenRCX
    "src/rcx/src/*.cpp",
    "src/rcx/src/*.h",
    #TritonRoute
    "src/drt/src/*.cpp",
    "src/drt/src/*.h",
    "src/drt/src/**/*.h",
    "src/drt/src/**/*.cpp",
    "src/drt/src/**/*.cc",
    #PDNSim
    "src/psm/src/*.cpp",
    "src/psm/src/*.h",
    #PartitionManager
    "src/par/src/*.cpp",
    #PDNGen
    "src/pdn/src/*.cc",
    "src/pdn/src/*.cpp",
    "src/pdn/src/*.h",
    #STT
    "src/stt/src/*.cpp",
    "src/stt/src/*.h",
    "src/stt/src/pdr/src/*.h",
    "src/stt/src/pdr/src/*.cpp",
    "src/stt/src/flt/*.cpp",
    #mpl
    "src/mpl/src/*.cpp",
    "src/mpl/src/*.h",
    #RMP
    "src/rmp/src/*.cpp",
    #Distributed
    "src/dst/src/*.cc",
    "src/dst/src/*.h",
    #Dpo
    "src/dpo/src/*.cpp",
    "src/dpo/src/*.cxx",
    "src/dpo/src/*.h",
    #pad
    "src/pad/src/*.cpp",
    "src/pad/src/*.h",
    #upf
    "src/upf/src/*.cpp",
    #utl
    "src/utl/*.h",
    #dft
    "src/dft/src/**/*.cpp",
    "src/dft/src/**/*.hh",
]
