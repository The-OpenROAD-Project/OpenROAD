# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, Precision Innovations Inc.

"""Rule to provide build-time notifications."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _notification_impl(ctx):
    if not ctx.attr.is_opt:
        # buildifier: disable=print
        print("\n" + "=" * 80 + "\n" +
              "  NOTIFICATION: Use --config=opt to build with LTO (Link Time Optimization)\n" +
              "                and get better performance at the expense of longer build time.\n" +
              "=" * 80 + "\n")
    return [CcInfo()]

notification_rule = rule(
    implementation = _notification_impl,
    attrs = {
        # Set to True when the build configuration enables LTO (via --config=opt).
        # When True, the notification message is suppressed.
        "is_opt": attr.bool(default = False),
    },
)
