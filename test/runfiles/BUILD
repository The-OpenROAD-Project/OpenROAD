# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_shell//shell:sh_test.bzl", "sh_test")

# Prove that the unsetenv(RUNFILES_DIR) fix in //bazel:tcl_library_init
# is necessary: this Bazel test has RUNFILES_DIR pointing to its own
# runfiles tree, yet OpenROAD must still find *its* TCL library.
sh_test(
    name = "runfiles_env_test",
    srcs = ["runfiles_env_test.sh"],
    args = ["$(rootpath //:openroad)"],
    data = ["//:openroad"],
)
