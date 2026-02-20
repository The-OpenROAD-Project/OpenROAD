# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "dpl",
    srcs = [
        "src/CheckPlacement.cpp",
        "src/DecapPlacement.cpp",
        "src/FillerPlacement.cpp",
        "src/Opendp.cpp",
        "src/OptMirror.cpp",
        "src/Optdp.cpp",
        "src/Place.cpp",
        "src/PlacementDRC.cpp",
        "src/PlacementDRC.h",
        "src/dbToOpendp.cpp",
        "src/graphics/DplObserver.h",
        "src/infrastructure/Coordinates.h",
        "src/infrastructure/DecapObjects.h",
        "src/infrastructure/Grid.cpp",
        "src/infrastructure/Grid.h",
        "src/infrastructure/Objects.cpp",
        "src/infrastructure/Objects.h",
        "src/infrastructure/Padding.cpp",
        "src/infrastructure/Padding.h",
        "src/infrastructure/architecture.cxx",
        "src/infrastructure/architecture.h",
        "src/infrastructure/detailed_segment.h",
        "src/infrastructure/network.cxx",
        "src/infrastructure/network.h",
        "src/legalize_shift.cxx",
        "src/legalize_shift.h",
        "src/objective/detailed_abu.cxx",
        "src/objective/detailed_abu.h",
        "src/objective/detailed_displacement.cxx",
        "src/objective/detailed_displacement.h",
        "src/objective/detailed_hpwl.cxx",
        "src/objective/detailed_hpwl.h",
        "src/objective/detailed_objective.h",
        "src/optimization/detailed.cxx",
        "src/optimization/detailed.h",
        "src/optimization/detailed_generator.h",
        "src/optimization/detailed_global.cxx",
        "src/optimization/detailed_global.h",
        "src/optimization/detailed_global_legacy.cxx",
        "src/optimization/detailed_global_legacy.h",
        "src/optimization/detailed_manager.cxx",
        "src/optimization/detailed_manager.h",
        "src/optimization/detailed_mis.cxx",
        "src/optimization/detailed_mis.h",
        "src/optimization/detailed_orient.cxx",
        "src/optimization/detailed_orient.h",
        "src/optimization/detailed_random.cxx",
        "src/optimization/detailed_random.h",
        "src/optimization/detailed_reorder.cxx",
        "src/optimization/detailed_reorder.h",
        "src/optimization/detailed_vertical.cxx",
        "src/optimization/detailed_vertical.h",
        "src/util/color.cxx",
        "src/util/color.h",
        "src/util/journal.cxx",
        "src/util/journal.h",
        "src/util/symmetry.h",
        "src/util/utility.cxx",
        "src/util/utility.h",
    ],
    hdrs = [
        "include/dpl/Opendp.h",
        "include/dpl/OptMirror.h",
    ],
    copts = [
        "-Isrc/dpl/src",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/odb",
        "//src/utl",
        "@boost.format",
        "@boost.geometry",
        "@boost.icl",
        "@boost.polygon",
        "@boost.property_tree",
        "@boost.random",
        "@boost.tokenizer",
        "@boost.utility",
        "@coin-or-lemon//:lemon",
        "@tk_tcl//:tcl",
    ],
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeOpendp.cpp",
        "src/graphics/DplObserver.h",
        "src/graphics/Graphics.cpp",
        "src/graphics/Graphics.h",
        "src/infrastructure/Coordinates.h",
        "src/infrastructure/Grid.h",
        "src/infrastructure/Objects.h",
        "src/infrastructure/architecture.h",
        "src/infrastructure/network.h",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/dpl/MakeOpendp.h",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":dpl",
        "//:ord",
        "//src/gui",
        "//src/odb",
        "//src/utl",
        "@boost.geometry",
        "@boost.icl",
        "@boost.polygon",
        "@boost.stacktrace",
        "@boost.utility",
        "@tk_tcl//:tcl",
    ],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/Opendp.tcl",
    ],
    char_array_name = "dpl_tcl_inits",
    namespace = "dpl",
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/Opendp.i",
        "//:error_swig",
    ],
    module = "dpl",
    namespace_prefix = "dpl",
    root_swig_src = "src/Opendp.i",
    swig_includes = [
        "src/dpl/src",
    ],
    deps = [
        "//src/odb:swig",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/dpl/Opendp.h",
        "src/Opendp-py.i",
        "//:error_swig-py",
    ],
    module = "dpl_py",
    root_swig_src = "src/Opendp-py.i",
    swig_includes = [
        "include",
        "src",
    ],
    deps = [
        "//src/odb:swig-py",
    ],
)
