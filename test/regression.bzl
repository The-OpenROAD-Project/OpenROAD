# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors

"""Instantiate a regression test based on .py or .tcl
files using resources in //test:regression_resources.

Also provides doc_check_test for lightweight Python-only
documentation tests that do not require the OpenROAD binary,
and messages_txt for generating messages.txt from source files."""

def _regression_test_impl(ctx):
    # Declare the test script output
    test_script = ctx.actions.declare_file(ctx.label.name + "_test.sh")

    # Determine test type: use explicit type if provided, otherwise infer from extension
    if ctx.attr.test_type:
        test_type = ctx.attr.test_type
    else:
        test_type = "python" if ctx.file.test_file.path.endswith(".py") else "tcl"

    # Generate the test script
    ctx.actions.write(
        output = test_script,
        content = """
#!/usr/bin/env bash
set -ex
export TEST_NAME_BAZEL={TEST_NAME_BAZEL}
export TEST_FILE={TEST_FILE}
export TEST_TYPE={TEST_TYPE}
export OPENROAD_EXE={OPENROAD_EXE}
export REGRESSION_TEST={REGRESSION_TEST}
export TEST_CHECK_LOG={TEST_CHECK_LOG}
export TEST_CHECK_PASSFAIL={TEST_CHECK_PASSFAIL}
exec "{bazel_test_sh}" "$@"
""".format(
            bazel_test_sh = ctx.file.bazel_test_sh.short_path,
            TEST_NAME_BAZEL = ctx.attr.test_name,
            TEST_FILE = ctx.file.test_file.short_path,
            TEST_TYPE = test_type,
            OPENROAD_EXE = ctx.executable.openroad.short_path,
            REGRESSION_TEST = ctx.file.regression_test.short_path,
            TEST_CHECK_LOG = "True" if ctx.attr.check_log else "False",
            TEST_CHECK_PASSFAIL = "True" if ctx.attr.check_passfail else "False",
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
        "bazel_test_sh": attr.label(
            doc = "The Bazel test shell script.",
            allow_single_file = True,
        ),
        "check_log": attr.bool(
            doc = "Diff the output log against <test_name>.ok",
            default = True,
        ),
        "check_passfail": attr.bool(
            doc = "Check the output log contains pass or OK in the last line",
            default = False,
        ),
        "data": attr.label_list(
            doc = "Additional test files required for the test.",
            allow_files = True,
        ),
        "openroad": attr.label(
            doc = "The OpenROAD executable.",
            executable = True,
            # Avoid building OpenROAD twice with "bazelisk test -c opt ..."
            #
            # OpenROAD is used to build more stuff in bazel-orfs,
            # hence we want the "exec" (host) configuration.
            cfg = "exec",
        ),
        "regression_test": attr.label(
            doc = "The regression test script.",
            allow_single_file = True,
        ),
        "test_file": attr.label(
            doc = "The primary test file (e.g., .tcl or .py).",
            allow_single_file = True,
        ),
        "test_name": attr.string(
            doc = "The name of the test.",
            mandatory = True,
        ),
        "test_type": attr.string(
            doc = "Test type: 'tcl', 'python' (via openroad), or 'standalone_python'. Auto-detected if not specified.",
            default = "",
        ),
    },
    executable = True,
    test = True,
)

def _doc_check_test_impl(ctx):
    """Lightweight test rule for pure-Python doc checks (no OpenROAD binary)."""
    test_script = ctx.actions.declare_file(ctx.label.name + "_test.sh")

    ctx.actions.write(
        output = test_script,
        content = """
#!/bin/bash
set -ex
export TEST_NAME_BAZEL={TEST_NAME_BAZEL}
export TEST_FILE={TEST_FILE}
export TEST_TYPE=standalone_python
export OPENROAD_EXE=
export REGRESSION_TEST={REGRESSION_TEST}
export TEST_CHECK_LOG={TEST_CHECK_LOG}
export TEST_CHECK_PASSFAIL=False
exec "{bazel_test_sh}" "$@"
""".format(
            bazel_test_sh = ctx.file.bazel_test_sh.short_path,
            TEST_NAME_BAZEL = ctx.attr.test_name,
            TEST_FILE = ctx.file.test_file.short_path,
            REGRESSION_TEST = ctx.file.regression_test.short_path,
            TEST_CHECK_LOG = "True" if ctx.attr.check_log else "False",
        ),
        is_executable = True,
    )

    return DefaultInfo(
        executable = test_script,
        runfiles = ctx.runfiles(
            files = ctx.files.data + [
                ctx.file.test_file,
                ctx.file.bazel_test_sh,
                ctx.file.regression_test,
            ],
        ),
    )

doc_check_rule_test = rule(
    implementation = _doc_check_test_impl,
    attrs = {
        "bazel_test_sh": attr.label(
            doc = "The Bazel test shell script.",
            allow_single_file = True,
        ),
        "check_log": attr.bool(
            doc = "Diff the output log against <test_name>.ok",
            default = True,
        ),
        "data": attr.label_list(
            doc = "Additional test files required for the test.",
            allow_files = True,
        ),
        "regression_test": attr.label(
            doc = "The regression test script.",
            allow_single_file = True,
        ),
        "test_file": attr.label(
            doc = "The primary test file (.py).",
            allow_single_file = True,
        ),
        "test_name": attr.string(
            doc = "The name of the test.",
            mandatory = True,
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

def doc_check_test(name, **kwargs):
    """Macro for lightweight Python doc check tests (no OpenROAD dependency).

    These tests validate documentation consistency (README, messages, man pages)
    and run in seconds without any C++ compilation.

    Args:
        name: The base name of the test (e.g., "cts_readme_msgs_check").
        **kwargs: Additional keyword arguments passed to doc_check_rule_test.
    """
    test_file = name + ".py"
    data = _pop(kwargs, "data", [])
    tags = _pop(kwargs, "tags", [])

    doc_check_rule_test(
        name = name + "-py_test",
        test_file = test_file,
        test_name = name,
        data = native.glob([name + ".*"]) + [
            "extract_utils.py",
            "manpage.py",
            "md_roff_compat.py",
        ] + data,
        bazel_test_sh = "//test:bazel_test.sh",
        regression_test = "//test:regression_test.sh",
        tags = tags + ["doc_check"],
        **kwargs
    )

def messages_txt(name = "messages_txt", src_patterns = None, visibility = None):
    """Generate messages.txt from source files using find_messages.py.

    Replaces per-module genrule boilerplate with a single macro call.

    Args:
        name: Target name (default: "messages_txt").
        src_patterns: Glob patterns for source files. Defaults to all
            common C++/Tcl extensions. Override for modules with
            non-standard layouts (e.g., recursive globs for odb).
        visibility: Bazel visibility.
    """
    if src_patterns == None:
        src_patterns = [
            "src/*.cc",
            "src/*.cpp",
            "src/*.h",
            "src/*.hh",
            "src/*.tcl",
        ]

    native.genrule(
        name = name,
        srcs = native.glob(src_patterns, allow_empty = True),
        outs = ["messages.txt"],
        cmd = "$(PYTHON3) $(location //etc:find_messages.py) -d $$(dirname $$(echo $(SRCS) | tr ' ' '\\n' | head -1)) > $@",
        toolchains = ["@rules_python//python:current_py_toolchain"],
        tools = ["//etc:find_messages.py"],
        visibility = visibility,
    )

def regression_test(
        name,
        **kwargs):
    """Macro to instantiate a regression test for a given test name

    Automatically detects test files and configures the test rule.

    Args:
        name: The base name of the test (without extension).
        **kwargs: Additional keyword arguments to pass to the regression_rule_test.
    """

    # TODO: we should _not_ have the magic to figure out if tcl or py exists
    # in here but rather in the BUILD file and just pass the resulting
    # name = "foo-tcl", test_file = "foo.tcl" to this regression test macro.
    test_files = native.glob(
        [name + "." + ext for ext in [
            "py",
            "tcl",
        ]],
        allow_empty = True,  # Allow to be empty; see also TODO above.
    )
    if not test_files:
        fail("No test file found for " + name)
    data = _pop(kwargs, "data", [])
    size = _pop(kwargs, "size", "small")
    test_type = _pop(kwargs, "test_type", "")
    tags = _pop(kwargs, "tags", [])
    for test_file in test_files:
        ext = test_file.split(".")[-1]

        # Python tests that need openroad -python are disabled because
        # the Bazel build doesn't include Python embedding support.
        # Tests with test_type="standalone_python" can still run.
        is_openroad_python_test = ext == "py" and test_type != "standalone_python"
        test_tags = tags + (["manual"] if is_openroad_python_test else [])
        regression_rule_test(
            name = name + "-" + ext + "_test",
            test_file = test_file,
            test_name = name,
            test_type = test_type,
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
            tags = test_tags,
            **kwargs
        )
