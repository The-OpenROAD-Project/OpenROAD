# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2026, The OpenROAD Authors

# OpenROAD no longer builds with CMake. This file only stops a `cmake`
# invocation with a pointer to the Bazel build.

cmake_minimum_required(VERSION 3.16)
project(OpenROAD NONE)

message(FATAL_ERROR [=[
The CMake build of OpenROAD was removed on November 1, 2026. Bazel is the only supported build system:

  bazelisk build //:openroad

See docs/user/Bazel.md or https://openroad.readthedocs.io/en/latest/user/Bazel.html
]=])
