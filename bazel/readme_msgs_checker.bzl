# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Runs each module's README/messages check with shared Python scripts."""

def _readme_msgs_check_impl(ctx):
    repository = ctx.label.repo_name or ctx.workspace_name
    runfiles_package = repository + "/" + ctx.label.package
    test_file = "../" + runfiles_package + "/" + ctx.attr.test_name + ".py"
    scripts = {}
    for source in ctx.files._scripts:
        filename = source.basename
        if filename == "readme_msgs_check.py":
            filename = ctx.attr.test_name + ".py"
        scripts[runfiles_package + "/" + filename] = source

    test_script = ctx.actions.declare_file(ctx.label.name + "_test.sh")
    ctx.actions.write(
        output = test_script,
        content = """#!/usr/bin/env bash
set -ex
export TEST_NAME_BAZEL={test_name}
export TEST_FILE={test_file}
export TEST_TYPE=standalone_python
export OPENROAD_EXE=
export REGRESSION_TEST={regression_test}
export TEST_CHECK_LOG=True
export TEST_CHECK_PASSFAIL=False
exec "{bazel_test_sh}" "$@"
""".format(
            test_name = ctx.attr.test_name,
            test_file = test_file,
            regression_test = ctx.file._regression_test.short_path,
            bazel_test_sh = ctx.file._bazel_test_sh.short_path,
        ),
        is_executable = True,
    )

    # Keep the module-local paths expected by the existing regression harness
    # without maintaining copies or symlinks in each source directory.
    runfiles = ctx.runfiles(
        files = [
            ctx.file._bazel_test_sh,
            ctx.file._regression_test,
        ] + ctx.files.data,
        root_symlinks = scripts,
    ).merge_all([
        dep[DefaultInfo].default_runfiles
        for dep in ctx.attr.data
        if DefaultInfo in dep
    ])
    return DefaultInfo(executable = test_script, runfiles = runfiles)

_readme_msgs_check_test = rule(
    implementation = _readme_msgs_check_impl,
    attrs = {
        "data": attr.label_list(allow_files = True),
        "test_name": attr.string(mandatory = True),
        "_bazel_test_sh": attr.label(
            default = "//test:bazel_test.sh",
            allow_single_file = True,
        ),
        "_regression_test": attr.label(
            default = "//test:regression_test.sh",
            allow_single_file = True,
        ),
        "_scripts": attr.label_list(
            default = [
                "//docs/src/scripts:extract_utils.py",
                "//docs/src/scripts:manpage.py",
                "//docs/src/scripts:md_roff_compat.py",
                "//docs/src/scripts:readme_msgs_check.py",
            ],
            allow_files = True,
        ),
    },
    test = True,
)

def readme_msgs_check_test(name, data = [], tags = [], **kwargs):
    """Checks README/man-page generation and compares its log with `<name>.ok`.

    Args:
        name: The base test name, e.g. "ant_readme_msgs_check".
        data: The module's doc_files and generated messages_txt targets.
        tags: Additional test tags.
        **kwargs: Common test attributes, such as visibility.
    """
    _readme_msgs_check_test(
        name = name + "-py_test",
        test_name = name,
        data = [name + ".ok"] + data,
        tags = tags + ["doc_check"],
        **kwargs
    )
