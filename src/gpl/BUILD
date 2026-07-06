# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_python//python:defs.bzl", "py_library")
load("//bazel:python_wrap_cc.bzl", "PYTHON_EXTENSION_LINKOPTS", "PYTHON_STABLE_API_DEFINE", "python_wrap_cc")
load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")
load("//bazel/gpu:copts.bzl", "CUDA_COPTS")
load("//bazel/gpu:defs.bzl", "GPU_ONLY")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# Headers owned by :gpl but also consumed by :ui and GPL tests. Keep them
# visible only to //src/gpl and its tests.
cc_library(
    name = "gpl_private_hdrs",
    hdrs = [
        "src/fft.h",
        "src/nesterovBase.h",
        "src/nesterovPlace.h",
        "src/placerBase.h",
        "src/point.h",
        "src/routeBase.h",
    ],
    includes = ["src"],
    visibility = [
        "//src/gpl/test:__pkg__",
        "//visibility:private",
    ],
    deps = [
        "//src/odb/src/db",
        "//src/utl",
        "@boost.unordered",
    ],
)

# The Kokkos/CUDA backends plus fft.cpp. fft.cpp itself includes only
# Kokkos-free PIMPL headers, but it is compiled as CUDA here to mirror
# the CMake flow's set_source_files_properties(... LANGUAGE CUDA) list in
# src/gpl/CMakeLists.txt, keeping the two builds behaviorally identical.
# hpwl.cpp stays a plain C++ TU in :gpl, also mirroring CMake.
cc_library(
    name = "gpl_kokkos",
    srcs = [
        "include/gpl/Replace.h",
        "src/backendContext.h",
        "src/densityGradientBackend.h",
        "src/fft.cpp",
        "src/fft.h",
        "src/fftBackend.h",
        "src/gpu/cellHandleHelpers.h",
        "src/gpu/dct.cpp",
        "src/gpu/dct.h",
        "src/gpu/densityOp.cpp",
        "src/gpu/densityOp.h",
        "src/gpu/deviceState.cpp",
        "src/gpu/deviceState.h",
        "src/gpu/deviceState_kokkos.h",
        "src/gpu/gpuDensityGradientBackend.cpp",
        "src/gpu/gpuDensityGradientBackend.h",
        "src/gpu/gpuFftBackend.cpp",
        "src/gpu/gpuFftBackend.h",
        "src/gpu/gpuHpwlBackend.cpp",
        "src/gpu/gpuHpwlBackend.h",
        "src/gpu/gpuRuntime.cpp",
        "src/gpu/gpuRuntime.h",
        "src/gpu/gpuWirelengthGradientBackend.cpp",
        "src/gpu/gpuWirelengthGradientBackend.h",
        "src/gpu/kokkosUtil.h",
        "src/gpu/nesterovDeviceContext.cpp",
        "src/gpu/nesterovDeviceContext.h",
        "src/gpu/nesterovDeviceState.h",
        "src/gpu/nesterovOp.cpp",
        "src/gpu/nesterovOp.h",
        "src/gpu/poissonSolver.cpp",
        "src/gpu/poissonSolver.h",
        "src/gpu/regionDensityField.cpp",
        "src/gpu/regionDensityField.h",
        "src/gpu/regionDensityField_kokkos.h",
        "src/gpu/wirelengthOp.cpp",
        "src/gpu/wirelengthOp.h",
        "src/hpwlBackend.h",
        "src/wirelengthGradientBackend.h",
    ],
    additional_compiler_inputs = ["@cuda_local//:compiler_inputs"],
    copts = CUDA_COPTS,
    # Kokkos/CUDA headers have no module maps, and clang's CUDA mode does
    # not support header parsing/modules.
    features = [
        "-layering_check",
        "-parse_headers",
    ],
    includes = ["include"],
    local_defines = ["ENABLE_GPU"],
    # Keep wildcard builds (bazel build //...) CPU-clean; this target is
    # only reachable through :gpl under --//:gpu=cuda.
    tags = ["manual"],
    target_compatible_with = GPU_ONLY,
    # Package-private except for the GPU unit tests, which link the
    # kernel launchers directly.
    visibility = ["//src/gpl/test:__pkg__"],
    deps = [
        ":gpl_private_hdrs",
        "//src/odb/src/db",
        "//src/utl",
        "@boost.unordered",
        "@kokkos",
        "@kokkos_fft",
        "@openmp",
    ],
)

cc_library(
    name = "gpl",
    srcs = [
        "src/AbstractGraphics.cpp",
        "src/backendContext.h",
        "src/clockBase.cpp",
        "src/clockBase.h",
        "src/densityGradient.cpp",
        "src/densityGradientBackend.h",
        "src/fft.h",
        "src/fftBackend.h",
        "src/fftsg.cpp",
        "src/fftsg2d.cpp",
        "src/gpu/deviceState.h",
        "src/gpu/nesterovDeviceContext.h",
        "src/gpu/regionDensityField.h",
        "src/graphicsNone.cpp",
        "src/hpwl.cpp",
        "src/hpwlBackend.h",
        "src/initialPlace.cpp",
        "src/initialPlace.h",
        "src/mbff.cpp",
        "src/nesterovBase.cpp",
        "src/nesterovPlace.cpp",
        "src/placerBase.cpp",
        "src/replace.cpp",
        "src/routeBase.cpp",
        "src/solver.cpp",
        "src/solver.h",
        "src/timingBase.cpp",
        "src/timingBase.h",
        "src/wirelengthGradient.cpp",
        "src/wirelengthGradientBackend.h",
    ] + select({
        "//:gpu_cuda": [
            # Kokkos-free GPU backend headers included by the factory TUs
            # (densityGradient.cpp, hpwl.cpp, wirelengthGradient.cpp,
            # nesterovBase.cpp) under #ifdef ENABLE_GPU.
            "src/gpu/gpuDensityGradientBackend.h",
            "src/gpu/gpuHpwlBackend.h",
            "src/gpu/gpuRuntime.h",
            "src/gpu/gpuWirelengthGradientBackend.h",
        ],
        "//conditions:default": [
            # On the GPU build fft.cpp moves to :gpl_kokkos, where it is
            # compiled as CUDA; on the CPU build it is a plain C++ TU here.
            "src/fft.cpp",
        ],
    }),
    hdrs = [
        "include/gpl/Replace.h",
        "src/AbstractGraphics.h",
        "src/graphicsNone.h",
        "src/mbff.h",
    ],
    copts = [
        "-fopenmp",
    ],
    includes = [
        "include",
    ],
    local_defines = select({
        "//:gpu_cuda": ["ENABLE_GPU"],
        "//conditions:default": [],
    }),
    deps = [
        ":gpl_private_hdrs",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/grt",
        "//src/odb/src/db",
        "//src/rsz",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@abseil-cpp//absl/container:inlined_vector",
        "@boost.polygon",
        "@boost.random",
        "@coin-or-lemon//:lemon",
        "@eigen",
        "@openmp",
        "@or-tools//ortools/linear_solver",
        "@or-tools//ortools/linear_solver:linear_solver_base",
        "@or-tools//ortools/sat:cp_model",
        "@or-tools//ortools/util:sorted_interval_list",
    ] + select({
        "//:gpu_cuda": [":gpl_kokkos"],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "ui",
    srcs = [
        "src/MakeReplace.cpp",
        "src/graphicsImpl.cpp",
        "src/graphicsImpl.h",
        ":swig",
        ":tcl",
    ],
    hdrs = [
        "include/gpl/MakeReplace.h",
    ],
    copts = [
        "-Wno-missing-braces",  # from TCL swigging
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":gpl",
        ":gpl_private_hdrs",
        "//:ord",
        "//src/gui",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@boost.stacktrace",
        "@tcl_lang//:tcl",
    ],
)

# This target compiles the SWIG C++ wrapper into a shared library (.so)
# that Python can load dynamically.
cc_binary(
    name = "_gpl.so",
    srcs = [
        ":swig-py",
    ],
    defines = [PYTHON_STABLE_API_DEFINE],
    includes = ["src"],
    linkopts = PYTHON_EXTENSION_LINKOPTS,
    linkshared = True,
    deps = [
        "//src/gpl",
        "//src/odb/src/db",
        "//src/utl",
        "@boost.stacktrace",
        "@rules_python//python/cc:current_py_cc_headers",
    ],
)

# This packages the SWIG-generated Python wrapper (gpl.py) and the
# compiled C++ extension (_gpl.so) together.
py_library(
    name = "gpl_py",
    srcs = [":swig-py"],  # Use the .py output from the swig-py rule
    # The data attribute makes the .so file available at runtime.
    data = [":_gpl.so"],
    # This allows imports relative to the workspace root.
    imports = ["."],
    visibility = ["//visibility:public"],
)

tcl_encode(
    name = "tcl",
    srcs = [
        "src/replace.tcl",
    ],
    char_array_name = "gpl_tcl_inits",
    namespace = "gpl",
    visibility = ["//visibility:public"],
)

tcl_wrap_cc(
    name = "swig",
    srcs = [
        "src/replace.i",
        "//:error_swig",
        "//:options_swig",
    ],
    module = "gpl",
    namespace_prefix = "gpl",
    root_swig_src = "src/replace.i",
    swig_includes = [
        "src/gpl/src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/odb:swig",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/gpl/Replace.h",
        "src/replace-py.i",
        "//:error_swig-py",
    ],
    module = "gpl",
    root_swig_src = "src/replace-py.i",
    swig_includes = [
        "include",
        "src",
    ],
    visibility = ["//:__subpackages__"],
)

filegroup(
    name = "doc_files",
    srcs = [
        "README.md",
        "src/replace.tcl",
    ],
    visibility = ["//src/gpl/test:__pkg__"],
)

messages_txt(
    # The default patterns are not recursive, but find_messages.py -d (as
    # run by CMake/CI) walks src/ recursively — include src/gpu explicitly
    # so a numbered message added to a GPU backend cannot silently
    # diverge between the two builds' messages.txt.
    src_patterns = [
        "src/*.cc",
        "src/*.cpp",
        "src/*.h",
        "src/*.hh",
        "src/*.tcl",
        "src/gpu/*.cpp",
        "src/gpu/*.h",
    ],
    visibility = [
        "//docs:__pkg__",
        "//src/gpl/test:__pkg__",
    ],
)
