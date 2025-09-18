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

py_library(
    name = "helpers",
    srcs = ["helpers.py"],
    deps = [
        "//openroad:openroadpy",
    ],
    imports = ["."],
)

py_library(
    name = "gpl_aux",
    srcs = ["gpl_aux.py"],
    deps = [
        "//openroad:openroadpy",
    ],
    imports = ["."],
)

filegroup(
    name = "asap7pdk",
    srcs = glob([
        "asap7/*.lib.gz",
        "asap7/*.lib",
        "asap7/*.lef",
        "asap7/*.tcl",
    ])
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
    "simple01-td-tune",
    "simple01-uniform",
    "simple02",
    "simple03",
    "simple04",
    "simple05",
    "simple06",
    "simple07",
    "simple08",
    "simple09",
    "simple10",
]

[
py_test(
    name = test_name,
    srcs = [test_name + ".py"],
    deps = [
        "//openroad:openroadpy",
        ":helpers",
        ":gpl_aux",
    ],
    data = [
        ":test_resources"
    ]
)
for test_name in PY_TESTS]
