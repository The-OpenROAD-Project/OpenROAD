"""Instantiate a regression test based on .py or .tcl
files using resources in //test:regression_resources"""

def _pop(kwargs, key, default):
    """BUILD does not support kwargs, use None as a "kwargs at home" workaround"""
    if key in kwargs:
        popped = kwargs.pop(key)
        if popped != None:
            return popped
    return default

def regression_test(
        name,
        **kwargs):
    test_files = native.glob([name + "." + ext for ext in [
        # TODO once Python is supported, add the .py files to the
        # "py",
        "tcl",
    ]])
    for test_file in test_files:
        ext = test_file.split(".")[-1]
        native.sh_test(
            name = name + "-" + ext,
            # top showed me 50-400mByte of usage, so "enormous" for
            # long running tests, but the OpenROAD tests are generally
            # "small" by design.
            #
            # https://bazel.build/reference/be/common-definitions#test.size
            size = _pop(kwargs, "size", "small"),
            timeout = _pop(kwargs, "size", "moderate"),
            srcs = ["//test:bazel_test.sh"],
            args = [],
            data = [
                "//:openroad",
                "//test:regression_resources",
                "//test:regression_test.sh",
            ] + test_files + _pop(kwargs, "data", []),
            env = {
                "TEST_NAME_BAZEL": name,
                "TEST_FILE": "$(location {test_file})".format(test_file = test_file),
                "TEST_FILES_BAZEL": " ".join(["$(location {file})".format(file = file) for file in test_files]),
                "OPENROAD_EXE": "$(location //:openroad)",
                "REGRESSION_TEST": "$(location //test:regression_test.sh)",
            },
            **kwargs
        )
