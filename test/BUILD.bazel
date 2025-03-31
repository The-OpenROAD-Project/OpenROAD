[sh_test(
    name = test,
    # top showed me 50-400mByte of usage, so "enormous"...
    #
    # https://bazel.build/reference/be/common-definitions#test.size
    size = "enormous",
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
        "TEST_EXT": "tcl",
    },
    # FIXME bazel test ... should run these tests in optimized mode only
    # as they take hours instead of minutes with unoptimized OpenROAD
    tags = ["optonly"],
) for test in [file.split(".")[0] for file in glob(
    # TODO add python tests too
    ["*.tcl"],
    exclude = ["regression.tcl"],
)]]
