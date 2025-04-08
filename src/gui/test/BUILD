load("//test:regression.bzl", "regression_test")

# From CMakeLists.txt or_integration_tests(TESTS
ALL_TESTS = [
    "supported",
]

[
    filegroup(
        name = test_name + "_resources",
        data = glob(
            [
                test_name + ".*",
            ],
        ),
    )
    for test_name in ALL_TESTS
]

[
    regression_test(
        name = test_name,
        data = [":" + test_name + "_resources"],
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]
