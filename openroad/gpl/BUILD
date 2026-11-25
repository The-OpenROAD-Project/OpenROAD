# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel:python_wrap_cc.bzl", "python_wrap_cc", "python_wrap_library")
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
# visible only to //openroad/gpl and its tests.
cc_library(
    name = "gpl_private_hdrs",
    hdrs = [
        "src/fft.h",
        "src/fftBackend.h",
        "src/nesterovBase.h",
        "src/nesterovPlace.h",
        "src/placerBase.h",
        "src/point.h",
        "src/routeBase.h",
    ],
    includes = ["src"],
    visibility = [
        "//openroad/gpl/test:__pkg__",
        "//visibility:private",
    ],
    deps = [
        "//bazel/kokkos",
        # TODO: we use headers provided by :gpl, but that reveals cyclic dep
        #":gpl",
        "//openroad/odb/src/db",
        "//openroad/utl",
        "@boost.random",
        "@boost.unordered",
    ],
)

# The Kokkos/CUDA device code plus fft.cpp and kokkosRuntime.cpp. Only TUs
# that launch device kernels belong here: everything in this target goes
# through the CUDA device pass. fft.cpp itself includes only Kokkos-free
# PIMPL headers, but it is compiled as CUDA along with them.
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
        "src/kokkosRuntime.cpp",
        "src/kokkosRuntime.h",
        "src/placerBase.h",
        "src/point.h",
        "src/routeBase.h",
        "src/wirelengthGradientBackend.h",
    ],
    additional_compiler_inputs = [
        "//bazel/gpu:cuda_placement_new.h",
        "@cuda_local//:compiler_inputs",
    ],
    copts = CUDA_COPTS,
    # Kokkos/CUDA headers have no module maps, and clang's CUDA mode does
    # not support header parsing/modules.
    features = [
        "-layering_check",
        "-parse_headers",
    ],
    # "src" lets the GPU sources include the gpl src/ headers listed above
    # by bare name (nesterovBase.h, fftBackend.h, ...). :gpl_impl also
    # exports this path but cannot be a dep here (cycle).
    includes = [
        "include",
        "src",
    ],
    local_defines = ["ENABLE_GPU"],
    # Keep wildcard builds (bazel build //...) CPU-clean; this target is
    # only reachable through :gpl under --//:gpu=cuda.
    tags = ["manual"],
    target_compatible_with = GPU_ONLY,
    # Package-private except for the GPU unit tests, which link the
    # kernel launchers directly.
    visibility = ["//openroad/gpl/test:__pkg__"],
    deps = [
        ":gpl_private_hdrs",
        #":gpl_impl",  # Needed, but causes cyclic dependencies.
        "//openroad/odb/src/db",
        "//openroad/utl",
        "@boost.random",
        "@boost.unordered",
        "@kokkos",
        "@kokkos_fft",
    ] + select({
        "@kokkos//:openmp": ["@openmp"],
        "//conditions:default": [],
    }),
)

# Placement TUs that use Kokkos only for host loops (HostRange) and reach
# the device code through the Kokkos-free PIMPL headers above. They include
# Kokkos_Core.hpp, so on the CUDA build they must be compiled as CUDA, but
# host-only: this skips the device pass over large host-only TUs and their
# dependencies (dbSta, grt, ppl, rsz, or-tools, lemon, ...), and keeps
# MBFF's Boost.Math dependencies, whose long-double operations are
# unsupported on the device, out of it.
cc_library(
    name = "gpl_kokkos_host",
    srcs = [
        "include/gpl/Replace.h",
        "src/AbstractGraphics.h",
        "src/backendContext.h",
        "src/densityGradient.cpp",
        "src/densityGradientBackend.h",
        "src/gpu/deviceState.h",
        "src/gpu/gpuDensityGradientBackend.h",
        "src/gpu/gpuHpwlBackend.h",
        "src/gpu/gpuRuntime.h",
        "src/gpu/gpuWirelengthGradientBackend.h",
        "src/gpu/nesterovDeviceContext.h",
        "src/gpu/regionDensityField.h",
        "src/hpwl.cpp",
        "src/hpwlBackend.h",
        "src/ioPinBase.cpp",
        "src/ioPinBase.h",
        "src/kokkosRuntime.h",
        "src/mbff.cpp",
        "src/mbff.h",
        "src/nesterovBase.cpp",
        "src/wirelengthGradient.cpp",
        "src/wirelengthGradientBackend.h",
    ],
    additional_compiler_inputs = [
        "//bazel/gpu:cuda_placement_new.h",
        "@cuda_local//:compiler_inputs",
    ],
    copts = CUDA_COPTS + ["--cuda-host-only"],
    features = [
        "-layering_check",
        "-parse_headers",
    ],
    includes = [
        "include",
        "src",
    ],
    local_defines = ["ENABLE_GPU"],
    tags = ["manual"],
    target_compatible_with = GPU_ONLY,
    deps = [
        ":gpl_kokkos",
        ":gpl_private_hdrs",
        "//openroad/dbSta",
        "//openroad/dbSta:dbNetwork",
        "//openroad/grt",
        "//openroad/odb/src/db",
        "//openroad/ppl",
        "//openroad/rsz",
        "//openroad/sta:opensta_lib",
        "//openroad/utl",
        "//third-party/core-math",
        "@abseil-cpp//absl/container:inlined_vector",
        "@boost.polygon",
        "@boost.random",
        "@boost.scope_exit",
        "@boost.unordered",
        "@coin-or-lemon//:lemon",
        "@eigen",
        "@kokkos",
        "@or-tools//ortools/linear_solver",
        "@or-tools//ortools/linear_solver:linear_solver_base",
        "@or-tools//ortools/sat:cp_model",
        "@or-tools//ortools/util:sorted_interval_list",
    ] + select({
        "@kokkos//:openmp": ["@openmp"],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "gpl_impl",
    srcs = [
        "src/AbstractGraphics.cpp",
        "src/backendContext.h",
        "src/clockBase.cpp",
        "src/clockBase.h",
        "src/densityGradientBackend.h",
        "src/fftsg.cpp",
        "src/fftsg2d.cpp",
        "src/gpu/deviceState.h",
        "src/gpu/nesterovDeviceContext.h",
        "src/gpu/regionDensityField.h",
        "src/graphicsNone.cpp",
        "src/hpwlBackend.h",
        "src/initialPlace.cpp",
        "src/initialPlace.h",
        "src/nesterovPlace.cpp",
        "src/placerBase.cpp",
        "src/powerBase.cpp",
        "src/replace.cpp",
        "src/routeBase.cpp",
        "src/solver.cpp",
        "src/solver.h",
        "src/timingBase.cpp",
        "src/timingBase.h",
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
            # On the GPU build fft.cpp and kokkosRuntime.cpp move to
            # :gpl_kokkos and the Kokkos host-loop TUs to :gpl_kokkos_host;
            # on the CPU build they are plain C++ TUs here.
            "src/fft.cpp",
            "src/kokkosRuntime.cpp",
            "src/kokkosRuntime.h",
            "src/densityGradient.cpp",
            "src/hpwl.cpp",
            "src/ioPinBase.cpp",
            "src/mbff.cpp",
            "src/nesterovBase.cpp",
            "src/wirelengthGradient.cpp",
        ],
    }),
    hdrs = [
        "include/gpl/Replace.h",
        "src/AbstractGraphics.h",
        "src/fft.h",
        "src/fftBackend.h",
        "src/graphicsNone.h",
        "src/ioPinBase.h",
        "src/mbff.h",
        "src/nesterovBase.h",
        "src/nesterovPlace.h",
        "src/placerBase.h",
        "src/point.h",
        "src/powerBase.h",
        "src/routeBase.h",
        "src/timingBase.h",
    ],
    copts = select({
        "@kokkos//:openmp": ["-fopenmp"],
        "//conditions:default": [],
    }),
    includes = [
        "include",
        "src",
    ],
    local_defines = select({
        "//:gpu_cuda": ["ENABLE_GPU"],
        "//conditions:default": [],
    }),
    visibility = [
        "//openroad/gpl:__subpackages__",
    ],
    deps = [
        ":gpl_private_hdrs",
        "//bazel/kokkos",
        "//openroad/dbSta",
        "//openroad/dbSta:dbNetwork",
        "//openroad/grt",
        "//openroad/odb/src/db",
        "//openroad/ppl",
        "//openroad/rsz",
        "//openroad/sta:opensta_lib",
        "//openroad/utl",
        "//third-party/core-math",
        "@abseil-cpp//absl/container:inlined_vector",
        "@boost.polygon",
        "@boost.random",
        "@boost.scope_exit",
        "@boost.unordered",
        "@coin-or-lemon//:lemon",
        "@eigen",
        "@or-tools//ortools/linear_solver",
        "@or-tools//ortools/linear_solver:linear_solver_base",
        "@or-tools//ortools/sat:cp_model",
        "@or-tools//ortools/util:sorted_interval_list",
    ] + select({
        "//:gpu_cuda": [
            ":gpl_kokkos",
            ":gpl_kokkos_host",
        ],
        "//conditions:default": [],
    }) + select({
        "@kokkos//:openmp": ["@openmp"],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "gpl",
    hdrs = [
        "include/gpl/Replace.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":gpl_impl",
    ],
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
        ":gpl_impl",
        "//:ord",
        "//openroad/odb/src/db",
        "//openroad/sta:opensta_lib",
        "//openroad/utl",
        "//openroad/web:core",
        "@tcl_lang//:tcl",
    ],
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
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//openroad/odb:swig",
    ],
)

python_wrap_cc(
    name = "swig-py",
    srcs = [
        "include/gpl/Replace.h",
        "src/replace-py.i",
        "//:error_swig-py",
    ],
    module = "gpl_py",
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
    visibility = ["//openroad/gpl/test:__pkg__"],
)

messages_txt(
    visibility = [
        "//docs:__pkg__",
        "//openroad/gpl/test:__pkg__",
    ],
)

python_wrap_library(
    name = "py_wrap",
    includes = ["src"],
    deps = [":gpl"],
)
