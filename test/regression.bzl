# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

"""Instantiate a regression test based on .py or .tcl
files using resources in //test:regression_resources"""

def _regression_test_impl(ctx):
    # Declare the test script output
    test_script = ctx.actions.declare_file(ctx.label.name + "_test.sh")

    # Generate the test script
    ctx.actions.write(
        output = test_script,
        content = """
#!/bin/bash
set -ex
export TEST_NAME_BAZEL={TEST_NAME_BAZEL}
export TEST_FILE={TEST_FILE}
export OPENROAD_EXE={OPENROAD_EXE}
export REGRESSION_TEST={REGRESSION_TEST}
exec "{bazel_test_sh}" "$@"
""".format(
            bazel_test_sh = ctx.file.bazel_test_sh.short_path,
            TEST_NAME_BAZEL = ctx.attr.test_name,
            TEST_FILE = ctx.file.test_file.short_path,
            OPENROAD_EXE = ctx.executable.openroad.short_path,
            REGRESSION_TEST = ctx.file.regression_test.short_path,
        ),
        is_executable = True,
    )

    # Return the test script as the executable
    return DefaultInfo(
        executable = test_script,
        runfiles = ctx.runfiles(
            transitive_files = depset(
                ctx.files.data + [
                    ctx.file.test_file,
                    ctx.file.bazel_test_sh,
                    ctx.file.regression_test,
                    ctx.executable.openroad,
                ],
                transitive = [
                    ctx.attr.openroad[DefaultInfo].default_runfiles.files,
                    ctx.attr.openroad[DefaultInfo].default_runfiles.symlinks,
                ],
            ),
        ),
    )

regression_rule_test = rule(
    implementation = _regression_test_impl,
    attrs = {
        "test_name": attr.string(
            doc = "The name of the test.",
            mandatory = True,
        ),
        "test_file": attr.label(
            doc = "The primary test file (e.g., .tcl or .py).",
            allow_single_file = True,
        ),
        "data": attr.label_list(
            doc = "Additional test files required for the test.",
            allow_files = True,
        ),
        "bazel_test_sh": attr.label(
            doc = "The Bazel test shell script.",
            allow_single_file = True,
        ),
        "openroad": attr.label(
            doc = "The OpenROAD executable.",
            executable = True,
            cfg = "target",
        ),
        "regression_test": attr.label(
            doc = "The regression test script.",
            allow_single_file = True,
        ),
    },
    executable = True,
    test = True,
)

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
    # TODO: we should _not_ have the magic to figure out if tcl or py exists
    # in here but rather in the BUILD file and just pass the resulting
    # name = "foo-tcl", test_file = "foo.tcl" to this regression test macro.
    test_files = native.glob(
        [name + "." + ext for ext in [
            # TODO once Python is supported, add the .py files to the
            # "py",
            "tcl",
        ]],
        allow_empty = True,  # Allow to be empty; see also TODO above.
    )
    data = _pop(kwargs, "data", [])
    size = _pop(kwargs, "size", "small")
    for test_file in test_files:
        ext = test_file.split(".")[-1]
        regression_rule_test(
            name = name + "-" + ext,
            test_file = test_file,
            test_name = name,
            data = [
                "//test:regression_resources",
            ] + test_files + data,
            bazel_test_sh = "//test:bazel_test.sh",
            openroad = "//:openroad",
            regression_test = "//test:regression_test.sh",
            # top showed me 50-400mByte of usage, so "enormous" for
            # long running tests, but the OpenROAD tests are generally
            # "small" by design.
            #
            # https://bazel.build/reference/be/common-definitions#test.size
            size = size,
            **kwargs
        )
