load("//test:regression.bzl", "regression_test")

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
    "incremental02",
    "nograd01",
    "simple01",
    "simple01-obs",
    "simple01-rd",
    "simple01-ref",
    "simple01-skip-io",
    "simple01-td",
    "simple01-td-tune",
    "simple01-uniform",
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

OPENROAD_COPTS = [
    "-fexceptions",
    "-Wno-error",
    "-Wall",
    "-Wextra",
    "-pedantic",
    "-Wno-cast-qual",  # typically from TCL swigging
    "-Wno-missing-braces",  # typically from TCL swigging
    "-Wredundant-decls",
    "-Wformat-security",
    "-Wno-sign-compare",
    "-Wno-unused-parameter",
    "-Wno-tautological-constant-out-of-range-compare",
    "-Wno-ctad-maybe-unsupported",
    "-Wno-nullability-extension",
    "-fopenmp",
]

cc_test(
    name = "mbff_test",
    srcs = ["mbff_test.cpp"],
    copts = OPENROAD_COPTS,
    data = [
        "library/test/test0.def",
        "library/test/test0.lef",
        "library/test/test0.lib",
    ],
    deps = [
        "@googletest//:gtest_main",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:status_matchers",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings:str_format",
        "@tk_tcl//:tcl",
        "//:openroad_lib",
        "//bazel:runfiles",
        "//src/gpl:gpl",
    ],
)
