load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

TESTS = [
    "check",
    "flute1",
    "flute_gcd",
    "parse_clocks",
    "pd1",
    "pd2",
    "pd_gcd",
]

filegroup(
    name = "test_resources",
    # overly broad glob, could be refined later, but
    # symlinks are cheap and OpenROAD binary changes, the common
    # use case is that all tests have to be re-run.
    srcs = glob(
        ["**/*"],
        exclude = [
            test + "." + ext
            for test in TESTS
            for ext in [
                "tcl",
                "py",
            ]
        ],
    ),
)

[regression_test(
    name = test_name,
    data = [":test_resources"],
) for test_name in TESTS]
