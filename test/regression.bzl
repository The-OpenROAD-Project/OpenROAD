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
export TEST_GOLDEN_FILE={TEST_GOLDEN_FILE}
export TEST_CHECK_LOG={TEST_CHECK_LOG}
export TEST_CHECK_PASSFAIL={TEST_CHECK_PASSFAIL}
export TEST_EXPECTED_EXIT_CODE={TEST_EXPECTED_EXIT_CODE}
exec "{bazel_test_sh}" "$@"
""".format(
            bazel_test_sh = ctx.file.bazel_test_sh.short_path,
            TEST_NAME_BAZEL = ctx.attr.test_name,
            TEST_FILE = ctx.file.test_file.short_path,
            TEST_TYPE = test_type,
            OPENROAD_EXE = ctx.executable.openroad.short_path,
            REGRESSION_TEST = ctx.file.regression_test.short_path,
            TEST_GOLDEN_FILE = ctx.file.golden_file.short_path if ctx.file.golden_file else "",
            TEST_CHECK_LOG = "True" if ctx.attr.check_log else "False",
            TEST_CHECK_PASSFAIL = "True" if ctx.attr.check_passfail else "False",
            TEST_EXPECTED_EXIT_CODE = str(ctx.attr.expected_exit_code),
        ),
        is_executable = True,
    )

    # Return the test script as the executable
    data_runfiles = [
        dep[DefaultInfo].default_runfiles
        for dep in ctx.attr.data
        if DefaultInfo in dep
    ]

    runfiles_files = [
        ctx.file.test_file,
        ctx.file.bazel_test_sh,
        ctx.file.regression_test,
        ctx.executable.openroad,
    ] + ctx.files.data
    if ctx.file.golden_file:
        runfiles_files.append(ctx.file.golden_file)

    return DefaultInfo(
        executable = test_script,
        runfiles = ctx.runfiles(
            files = runfiles_files,
        ).merge_all(data_runfiles + [ctx.attr.openroad[DefaultInfo].default_runfiles]),
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
        "expected_exit_code": attr.int(
            doc = "Expected command exit code for the regression.",
            default = 0,
        ),
        "golden_file": attr.label(
            doc = "Optional expected output file used for log diffing.",
            allow_single_file = True,
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

    data_runfiles = [
        dep[DefaultInfo].default_runfiles
        for dep in ctx.attr.data
        if DefaultInfo in dep
    ]

    return DefaultInfo(
        executable = test_script,
        runfiles = ctx.runfiles(
            files = [
                ctx.file.test_file,
                ctx.file.bazel_test_sh,
                ctx.file.regression_test,
            ] + ctx.files.data,
        ).merge_all(data_runfiles),
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

def _dedupe_list(items):
    """Return items with duplicates removed while preserving order."""
    seen = {}
    unique = []
    for item in items:
        if item in seen:
            continue
        seen[item] = True
        unique.append(item)
    return unique

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

def messages_txt(name = "messages_txt", src_patterns = None, extra_srcs = None, visibility = None):
    """Generate messages.txt from source files using find_messages.py.

    Replaces per-module genrule boilerplate with a single macro call.

    Args:
        name: Target name (default: "messages_txt").
        src_patterns: Glob patterns for source files. Defaults to all
            common C++/Tcl extensions. Override for modules with
            non-standard layouts (e.g., recursive globs for odb).
        extra_srcs: Additional source labels from other packages (e.g.,
            filegroups in sub-packages). When provided, files are passed
            directly to find_messages.py instead of directory walking.
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

    srcs = native.glob(src_patterns, allow_empty = True)
    if extra_srcs:
        srcs = srcs + extra_srcs

    # When extra_srcs are present, sources span multiple directories so
    # the dirname-of-first-src trick doesn't work.  Pass every file
    # path as a positional arg to find_messages.py instead.
    if extra_srcs:
        cmd = "$(PYTHON3) $(location //etc:find_messages.py) $(SRCS) > $@"
    else:
        cmd = "$(PYTHON3) $(location //etc:find_messages.py) -d $$(dirname $$(echo $(SRCS) | tr ' ' '\\n' | head -1)) > $@"

    native.genrule(
        name = name,
        srcs = srcs,
        outs = ["messages.txt"],
        cmd = cmd,
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

        test_data = [
            "//test:regression_resources",
        ] + test_files + data
        effective_test_type = test_type
        if ext == "py" and not effective_test_type:
            effective_test_type = "python"
            test_data += native.glob(["*.py"]) + ["//python/openroad:openroadpy"]
        test_data = _dedupe_list(test_data)

        regression_rule_test(
            name = name + "-" + ext + "_test",
            test_file = test_file,
            test_name = name,
            test_type = effective_test_type,
            data = test_data,
            bazel_test_sh = "//test:bazel_test.sh",
            openroad = "//:openroad",
            regression_test = "//test:regression_test.sh",
            # top showed me 50-400mByte of usage, so "enormous" for
            # long running tests, but the OpenROAD tests are generally
            # "small" by design.
            #
            # https://bazel.build/reference/be/common-definitions#test.size
            size = size,
            tags = tags,
            **kwargs
        )
