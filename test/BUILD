[sh_test(
    name = test,
    # top showed me 50-400mByte of usage, so "enormous" for
    # long running tests, but the OpenROAD tests are generally
    # "small" by design.
    #
    # https://bazel.build/reference/be/common-definitions#test.size
    size = "small",
    timeout = "moderate",
    srcs = ["bazel_test.sh"],
    args = [],
    data = [
        "//:openroad",
    ] +
    # TODO Refine glob later.
    #
    # This is a very broad glob, but most of the data in size
    # comes from .lib files and we can't know which pdk files
    # to include for a test given the name only.
    glob([
        "**/*",
    ]),
    env = {
        "TEST_NAME_BAZEL": test,
        "OPENROAD_EXE": "$(location //:openroad)",
    },
) for test in [
    # From CMakeLists.txt or_integration_tests(TESTS
    "error1",
    "get_core_die_areas",
    "place_inst",
    "upf_test",
    "upf_aes",
    # Bazel OpenROAD is not compiled with Python support yet
    #
    # "timing_api",
    # "timing_api_2",
    # "timing_api_3",
    # "timing_api_4",
    # "two_designs",
    #
    # TODO There are tests in this folder that were not enabled in
    # CMakeLists.txt, presumably because they are long running.
    # These should be added with tags = ["manual"], i.e not
    # run with 'bazel test ...', only when invoked explicitly
    # and the "size" set to "enormous".
    #
    #"aes_nangate45",
]]
