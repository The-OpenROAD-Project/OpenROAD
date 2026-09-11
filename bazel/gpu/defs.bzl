# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Shared select() helpers for targets that participate in the opt-in
GPU build (--config=gpu). See bazel/gpu/system_gpu.bzl for the backing
repositories and bazel/gpu/copts.bzl for the CUDA compile flags.
"""

# For target_compatible_with: the target only exists on a GPU build.
# Plain CPU wildcard runs (bazel test //...) report such targets as
# SKIPPED instead of failing.
GPU_ONLY = select({
    "//:gpu_cuda": [],
    "//conditions:default": ["@platforms//:incompatible"],
})

# For the env attribute of tests that compare against CPU golden logs:
# on a GPU build the GPU FFT backend is not bit-identical to the CPU
# Ooura FFT, so the ENABLE_GPU runtime gate (default on when compiled
# in; see gpl::gpuEnabled) is pinned off. On the default CPU build the
# branch is empty and the variable is never read. Mirrors the
# ENVIRONMENT injection in src/gpl/test/CMakeLists.txt.
GPU_ENV_OFF = select({
    "//:gpu_cuda": {"ENABLE_GPU": "0"},
    "//conditions:default": {},
})
