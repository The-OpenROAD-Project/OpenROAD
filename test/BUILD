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

[
    regression_test(
        name = name,
        tags = kwargs.get("tags", []),
    )
    for name, kwargs in (
        {test: {} for test in [
            # From CMakeLists.txt or_integration_tests(TESTS
            "error1",
            "get_core_die_areas",
            "place_inst",
            "upf_test",
            "upf_aes",
        ]} | {
            # Bazel OpenROAD is not compiled with Python support yet
            # leave as manual for now.
            test: {"tags": ["manual"]}
            for test in [
                "timing_api",
                "timing_api_2",
                "timing_api_3",
                "timing_api_4",
                "two_designs",
            ]
        } | {
            test: {
                "tags": ["manual"],
                "size": "enormous",
            }
            for test in [
                # TODO There are tests in this folder that were not enabled in
                # CMakeLists.txt, presumably because they are long running.
                # These should be added with tags = ["manual"], i.e not
                # run with 'bazel test ...', only when invoked explicitly
                # and the "size" set to "enormous".
                "aes_nangate45",
            ]
        }
    ).items()
]
