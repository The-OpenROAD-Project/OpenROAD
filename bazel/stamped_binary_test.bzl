# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Analysis test for version stamping on the public OpenROAD binary."""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")

def _stamped_binary_test_impl(ctx):
    env = analysistest.begin(ctx)
    stamp_inputs = [
        source.basename
        for action in analysistest.target_actions(env)
        if action.mnemonic == "StampVersion"
        for source in action.inputs.to_list()
    ]
    asserts.true(env, "stable-status.txt" in stamp_inputs, "The public binary must use the stable workspace status for its version.")
    return analysistest.end(env)

stamped_binary_test = analysistest.make(_stamped_binary_test_impl)
