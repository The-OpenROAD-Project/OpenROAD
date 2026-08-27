# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""One test target per corpus netlist, sharing a single linked runner.

The hierarchy suites run ~1280 netlists through two link modes each. As one
sharded cc_test that is one cache entry: touching any netlist reruns every
shard, because every shard depends on the whole corpus. Bazel caches per target,
not per test case, so per-case caching means per-case targets.

Two things make that affordable:

  * One runner binary. A cc_test per case would link the runner ~1280 times;
    these targets take a single cc_binary as data and select their case with
    HIER_CASES, so the link happens once per suite.
  * One netlist per target's runfiles. Narrowing `data` is what buys the
    caching -- targets that all glob the corpus are invalidated together no
    matter how many of them there are.

The corpus-wide checks (is the corpus loaded, are its TARGETS tags unique, is
the manifest well formed) cannot be per-case: they are built from the same rule
with no `cases`, which leaves the runner scanning the case directories, and an
inverted filter so the two halves partition the suite rather than overlap.
"""

def _hier_case_test_impl(ctx):
    script = ctx.actions.declare_file(ctx.label.name + ".sh")

    ctx.actions.write(
        output = script,
        content = """#!/bin/bash
set -eu
exec "{runner}" --gtest_filter='{filter}' "$@"
""".format(
            runner = ctx.executable.runner.short_path,
            filter = ctx.attr.gtest_filter,
        ),
        is_executable = True,
    )

    # A case's metadata travels in the environment, which is part of the test
    # action's key: editing one case's top module or XFAIL rows invalidates that
    # case and nothing else. It is not baked into the script above because an
    # XFAIL symptom is prose, and prose does not survive shell quoting.
    env = {"HIER_TOP_OVERRIDES": ctx.attr.top_overrides}
    if ctx.attr.cases:
        env["HIER_CASES"] = ",".join(ctx.attr.cases)

        # Set even when empty: a case with no known failures must not fall back
        # to the shared manifest, which would put every case back in one cache
        # bucket. The corpus-wide target leaves it unset and reads the manifest.
        env["HIER_EXPECTED_FAIL"] = "\n".join(ctx.attr.expected_fail)

    return [
        DefaultInfo(
            executable = script,
            runfiles = ctx.runfiles(
                files = [script] + ctx.files.data,
            ).merge(ctx.attr.runner[DefaultInfo].default_runfiles),
        ),
        RunEnvironmentInfo(environment = env),
    ]

hier_case_test = rule(
    implementation = _hier_case_test_impl,
    doc = "Runs part of a hierarchy suite: named cases, or everything else.",
    attrs = {
        "cases": attr.string_list(
            doc = "Corpus-relative names ('case.v', 'structural/case.v'). " +
                  "Empty leaves the runner scanning the case directories.",
        ),
        "data": attr.label_list(
            doc = "The netlists this target runs, and the XFAIL manifest.",
            allow_files = True,
        ),
        "expected_fail": attr.string_list(
            doc = "This case's XFAIL manifest rows, if it has any.",
        ),
        "gtest_filter": attr.string(
            doc = "Which tests to run.",
            default = "*MatchesInput*",
        ),
        "runner": attr.label(
            doc = "The suite binary, shared by every target of that suite.",
            executable = True,
            cfg = "target",
            mandatory = True,
        ),
        "top_overrides": attr.string(
            doc = "'<file>=<top>,...' for cases whose top module is not 'top'.",
            default = "",
        ),
    },
    executable = True,
    test = True,
)

def render_top_overrides(overrides):
    """Renders a file name -> top module dict as the HIER_TOP_OVERRIDES value.

    Args:
      overrides: file name -> top module.

    Returns:
      A '<file>=<top>,...' string.
    """
    return ",".join(["%s=%s" % (f, t) for f, t in overrides.items()])

def _target_name(prefix, case):
    return "".join([
        c if c.isalnum() else "_"
        for c in (prefix + "_" + case).elems()
    ])

def hier_case_tests(
        name,
        runner,
        netlists,
        subdir = "",
        top_overrides = {},
        expected_fail = {},
        data = [],
        size = "small"):
    """A test target per netlist. Returns the generated labels.

    Args:
      name: prefix for the generated target names.
      runner: the suite binary label.
      netlists: netlist labels, e.g. glob(["cpp/hier_cases/*.v"]).
      subdir: corpus-relative subdirectory prefix ("structural/", "inherited/").
      top_overrides: file name -> top module, for tops that are not "top".
      expected_fail: corpus-relative name -> its XFAIL manifest rows.
      data: shared inputs every case needs (the filegroups behind the
        inherited/ symlinks; not the manifest -- rows travel per case).
      size: bazel test size; a case is ~0.6s.

    Returns:
      The list of generated test target labels.
    """

    # A group that came back empty is a broken data dependency, which is how
    # these cases went unrun once already. With one target per netlist an empty
    # group is silent -- there is no target left to report it -- so it has to be
    # an error here, where the corpus-wide IsLoaded check can no longer see it.
    if not netlists:
        fail("no netlists for '%s%s'; check the glob" % (name, subdir))

    tests = []
    for netlist in netlists:
        file_name = netlist.rpartition("/")[2]
        case = subdir + file_name
        target = _target_name(name, case)
        top = top_overrides.get(file_name, "")
        hier_case_test(
            name = target,
            cases = [case],
            data = [netlist] + data,
            # Keyed the way each suite's manifest is: the structural manifest
            # keeps a subdirectory case's prefix, the conformance manifest keys
            # every case by its bare file name.
            expected_fail = expected_fail.get(
                case,
                expected_fail.get(file_name, []),
            ),
            runner = runner,
            size = size,
            top_overrides = "" if not top else "%s=%s" % (file_name, top),
        )
        tests.append(":" + target)
    return tests
