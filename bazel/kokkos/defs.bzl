# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Re-run an existing test with a different host Kokkos backend.

The host backend is a build setting (//bazel/kokkos:backend), so a plain
`bazel test` only ever exercises the default (openmp). kokkos_backend_test
applies a configuration transition to the wrapped test, rebuilding it and
its OpenROAD binary with the requested backend, and runs it unchanged.
"""

# Go through the //bazel/kokkos alias rather than naming the Kokkos
# repository, so the wrapper does not depend on where Kokkos comes from.
_BACKEND_SETTING = "//bazel/kokkos:backend"

def _backend_transition_impl(_settings, attr):
    return {_BACKEND_SETTING: attr.backend}

_backend_transition = transition(
    implementation = _backend_transition_impl,
    inputs = [],
    outputs = [_BACKEND_SETTING],
)

def _kokkos_backend_test_impl(ctx):
    test = ctx.attr.test
    if type(test) == "list":
        test = test[0]
    test_exe = ctx.executable.test

    # Tests must own their executable, so forward the wrapped one via a
    # symlink. Runfiles paths are configuration-independent (short_path),
    # so the wrapped script finds its inputs under our runfiles tree.
    out = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.symlink(
        output = out,
        target_file = test_exe,
        is_executable = True,
    )

    env = {}
    if RunEnvironmentInfo in test:
        env.update(test[RunEnvironmentInfo].environment)
    env.update(ctx.attr.env)

    return [
        DefaultInfo(
            executable = out,
            runfiles = ctx.runfiles(files = [test_exe]).merge(
                test[DefaultInfo].default_runfiles,
            ),
        ),
        RunEnvironmentInfo(environment = env),
    ]

_kokkos_backend_test = rule(
    implementation = _kokkos_backend_test_impl,
    attrs = {
        "backend": attr.string(
            doc = "Host Kokkos backend to build the wrapped test with.",
            mandatory = True,
            values = [
                "openmp",
                "serial",
                "threads",
            ],
        ),
        "env": attr.string_dict(
            doc = "Extra environment, merged over the wrapped test's.",
        ),
        "test": attr.label(
            doc = "The test target to re-run.",
            mandatory = True,
            executable = True,
            cfg = _backend_transition,
        ),
    },
    test = True,
)

def kokkos_backend_tests(tests, backends = ["serial", "threads"], **kwargs):
    """Instantiate <test>-<backend> for every test/backend pair.

    The host backend has no effect on the CUDA build (which links @kokkos),
    so these targets are skipped there.

    Args:
      tests: labels of existing test targets (e.g. ":core01-tcl_test").
      backends: host backends to exercise in addition to the default.
      **kwargs: forwarded to every generated test (size, tags, ...).
    """
    for test in tests:
        base = test.split(":")[-1]
        for backend in backends:
            _kokkos_backend_test(
                name = "{}-{}".format(base, backend),
                test = test,
                backend = backend,
                target_compatible_with = select({
                    "//:gpu_cuda": ["@platforms//:incompatible"],
                    "//conditions:default": [],
                }),
                **kwargs
            )
