load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_python//python:defs.bzl", "py_library", "py_test")
load("//bazel/gpu:copts.bzl", "CUDA_COPTS")
load("//bazel/gpu:defs.bzl", "GPU_ENV_OFF", "GPU_ONLY")
load("//test:regression.bzl", "doc_check_test", "regression_test")

package(features = ["layering_check"])

TESTS = [
    "ar01",
    "ar02",
    "clust01",
    "clust02",
    "clust03",
    "cluster_place01",
    "convergence01",
    "core01",
    "density01",
    "diverge01",
    "error01",
    "incremental01",
    "mbff_hier",
    "mbff_orig_name",
    "mbff_clock_power_off",
    "mbff_clock_power_on",
    "mt_invariance01",
    "nograd01",
    "simple01",
    "simple01-obs",
    "simple01-rd",
    "simple01-ref",
    "simple01-skip-io",
    "simple01-td",
    "simple01-td-net-percentage",
    "simple01-td-tune",
    "simple01-td-repair",
    "simple01-uniform",
    "simple01-vcts",
    "simple01-vcts-clamp",
    "simple01-vcts-noclk",
    "simple01-vcts-userlat",
    "simple02",
    "simple02-rd",
    "simple03",
    "simple03-rd",
    "simple04",
    "simple04-rd",
    "simple05",
    "simple06",
    "simple07",
    "simple08",
    "simple09",
    "simple10",
    "region01",
]

PASSFAIL_TESTS = [
    "incremental02",
    "place_ios01",
    "place_ios_top_layer",
]

# Longer-running regression tests (see regression_tests_large.tcl). These are
# tagged "manual" so they are excluded from the default `bazel test //...`
# wildcard and must be requested explicitly, e.g. via the :large_tests suite.
LARGE_TESTS = [
    "macro01",
    "macro02",
    "macro03",
    "medium01",
    "medium02",
    "medium03",
    "medium04",
    "medium05",
    "medium06",
    "large01",
    "large02",
]

# GPU-only PASSFAIL regressions, pinned to the GPU code path (mirrors
# the ENABLE_GPU=1 tests in src/gpl/test/CMakeLists.txt).
GPU_TESTS = [
    "region01_gpu",
    "region01_gpu_asym",
]

ALL_TESTS = TESTS + PASSFAIL_TESTS

py_library(
    name = "helpers",
    srcs = ["helpers.py"],
    imports = ["."],
    deps = [
        "//python/openroad:openroadpy",
    ],
)

filegroup(
    name = "asap7pdk",
    srcs = glob([
        "asap7/*.lib.gz",
        "asap7/*.lib",
        "asap7/*.lef",
        "asap7/*.tcl",
    ]),
)

filegroup(
    name = "test_resources",
    # overly broad glob, could be refined later, but
    # symlinks are cheap and OpenROAD binary changes, the common
    # use case is that all tests have to be re-run.
    srcs = glob(
        ["**/*"],
        exclude = [
            test + "." + ext
            for test in ALL_TESTS + LARGE_TESTS + GPU_TESTS
            for ext in [
                "tcl",
                "py",
            ]
        ],
    ),
)

[regression_test(
    name = test_name,
    check_log = False if test_name in PASSFAIL_TESTS else True,
    check_passfail = True if test_name in PASSFAIL_TESTS else False,
    data = [":test_resources"],
) for test_name in ALL_TESTS]

[regression_test(
    name = test_name,
    size = "enormous",
    data = [":test_resources"],
    tags = ["manual"],
) for test_name in LARGE_TESTS]

# Convenience suite to run all long-running tests explicitly:
#   bazel test //src/gpl/test:large_tests
test_suite(
    name = "large_tests",
    tags = ["manual"],
    tests = [":" + test_name + "-tcl_test" for test_name in LARGE_TESTS],
)

# ENABLE_GPU=1 overrides the GPU_ENV_OFF default that regression_test()
# applies to every other test here.
[regression_test(
    name = test_name,
    check_log = False,
    check_passfail = True,
    data = [":test_resources"],
    env = {"ENABLE_GPU": "1"},
    tags = ["gpu"],
    target_compatible_with = GPU_ONLY,
) for test_name in GPU_TESTS]

PY_TESTS = [
    "ar01",
    "ar02",
    "convergence01",
    "core01",
    "error01",
    "incremental01",
    "nograd01",
    "simple01",
    "simple01-obs",
    "simple01-ref",
    "simple01-skip-io",
    "simple01-td",
    "simple01-td-net-percentage",
    "simple01-td-tune",
    "simple01-td-repair",
    "simple01-uniform",
    "simple02",
    "simple03",
    "simple04",
    "simple05",
    "simple07",
    "simple08",
    "simple09",
    "simple10",
]

[
    py_test(
        name = test_name,
        srcs = [test_name + ".py"],
        data = [
            ":test_resources",
        ],
        env = GPU_ENV_OFF,
        tags = ["py"],
        deps = [
            ":helpers",
            "//python/openroad:openroadpy",
        ],
    )
    for test_name in PY_TESTS
]

cc_test(
    name = "gpl_fft_unittest",
    srcs = ["fft_test.cc"],
    env = GPU_ENV_OFF,
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        "//src/gpl",  # buildcleaner: keep: needs fixing gpl layering first.
        "//src/gpl:gpl_private_hdrs",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@spdlog",
    ],
)

# GPU FFT backend unit test (plain C++ TU; the GPU code arrives through
# //src/gpl's :gpl_kokkos dependency under --config=gpu).
cc_test(
    name = "fft_gpu_test",
    srcs = ["fft_gpu_test.cc"],
    env = {"ENABLE_GPU": "1"},
    features = ["-layering_check"],  # includes private headers
    linkstatic = True,
    tags = ["gpu"],
    target_compatible_with = GPU_ONLY,
    deps = [
        "//src/gpl",
        "//src/gpl:gpl_private_hdrs",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# GPU wirelength-kernel correctness test. Includes Kokkos-laden headers
# directly, so the TU itself is compiled as CUDA (mirrors the
# set_source_files_properties block in test/CMakeLists.txt).
cc_test(
    name = "wl_gpu_test",
    srcs = ["wl_gpu_test.cc"],
    additional_compiler_inputs = [
        "//bazel/gpu:cuda_placement_new.h",
        "@cuda_local//:compiler_inputs",
    ],
    copts = CUDA_COPTS,
    env = {"ENABLE_GPU": "1"},
    features = [
        "-layering_check",
        "-parse_headers",
    ],
    linkstatic = True,
    tags = ["gpu"],
    target_compatible_with = GPU_ONLY,
    deps = [
        "//src/gpl:gpl_kokkos",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mbff_test",
    srcs = ["mbff_test.cpp"],
    data = [
        "library/test/test0.lef",
        "library/test/test0.lib",
    ],
    env = GPU_ENV_OFF,
    deps = [
        "//src/ant",
        "//src/dbSta:dbNetwork",
        "//src/dbSta:dbReadVerilog",
        "//src/dpl",
        "//src/est",
        "//src/gpl",
        "//src/grt",
        "//src/odb/src/db",
        "//src/rsz",
        "//src/stt",
        "//src/tst",
        "//src/utl",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "placer_base_test",
    srcs = ["placer_base_test.cpp"],
    env = GPU_ENV_OFF,
    deps = [
        "//src/gpl",
        "//src/gpl:gpl_private_hdrs",
        "//src/odb/src/db",
        "//src/tst:db_fixture",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

py_test(
    name = "gpl_man_tcl_check",
    srcs = ["gpl_man_tcl_check.py"],
    data = ["//src/gpl:doc_files"],
    tags = [
        "doc_check",
        "py",
    ],
    deps = ["//docs/src/scripts:extract_utils"],
)

doc_check_test(
    name = "gpl_readme_msgs_check",
    data = [
        "//src/gpl:doc_files",
        "//src/gpl:messages_txt",
    ],
    visibility = ["//visibility:public"],
)

doc_check_test(
    name = "gpl_messages_txt_check",
    check_log = False,
    data = ["//src/gpl:messages_txt"],
)
