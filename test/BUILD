load("//test:regression.bzl", "regression_test")

exports_files(
    [
        "bazel_test.sh",
        "regression_test.sh",
    ],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "regression_resources",
    # TODO Refine glob later.
    #
    # This is a very broad glob, but most of the data in size
    # comes from .lib files and we can't know which pdk files
    # to include for a test given the name only.
    srcs = glob(["**/*"]),
    visibility = ["//visibility:public"],
)

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "error1",
    "get_core_die_areas",
    "place_inst",
    "upf_test",
    "upf_aes",
]

# Bazel OpenROAD is not compiled with Python support yet
# leave as manual for now.
PYTHON_TESTS = [
    "timing_api",
    "timing_api_2",
    "timing_api_3",
    "timing_api_4",
    "two_designs",
]

# TODO There are tests in this folder that were not enabled in
# CMakeLists.txt, presumably because they are long running.
# These should be added with tags = ["manual"], i.e not
# run with 'bazel test ...', only when invoked explicitly
# and the "size" set to "enormous".
BIG_TESTS = [
    "aes_nangate45",
    "aes_sky130hd.tcl",
    "aes_sky130hs.tcl",
    "gcd_sky130hd_fast_slow.tcl",
    "gcd_sky130hd.tcl",
    "gcd_sky130hs.tcl",
    "ibex_sky130hd.tcl",
    "ibex_sky130hs.tcl",
    "jpeg_sky130hd.tcl",
    "jpeg_sky130hs.tcl",
    "aes_nangate45.tcl",
    "gcd_nangate45.tcl",
    "tinyRocket_nangate45.tcl",
]

[
    regression_test(
        name = test_name,
        size = "enormous" if test_name in BIG_TESTS else "medium",
        tags = [] if test_name in COMPULSORY_TESTS else ["manual"],
    )
    for test_name in COMPULSORY_TESTS + PYTHON_TESTS + BIG_TESTS
]
