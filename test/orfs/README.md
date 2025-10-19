# ORFS smoke tests

A set of ORFS integration tests that runs in a few minutes suitable for inclusion in the local fast regression testing workflow prior to creating a PR:

    bazelisk test ...

## Updating RULES_JSON files

1. Run `bazelisk run //test/orfs/gcd:gcd_update` to update RULES_JSON file for a design. This will build and run OpenROAD to generate a new RULES_JSON file and update the RULES_JSON source file.
2. Create commit for RULES_JSON file

## Updating ORFS and bazel-orfs

`bazelisk run @bazel-orfs//:bump`, will find the latest bazel-orfs and ORFS docker image and update MODULE.bazel and MODULE.bazel.lock.

Note that this update is needed infrequently because the compability between ORFS and OpenROAD only needs to be considered for `bazelisk test/orfs/...` tests.

bazel-orfs is updated at the same time as ORFS with the command above. bazel-orfs must sometimes be updated due to changes in the ORFS interface(such as variables.yaml updates or names of output files).

# Tips on debugging bazel-orfs failures

## Search logs from top

Search for "error:" in logs from the top and you might find somethin like, or more specifically "error executing Action":

    [2025-07-15T21:13:48.963Z] (21:13:48) ERROR: /tmp/workspace/OpenROAD-Public_PR-7814-head/test/orfs/mock-array/BUILD:169:10: Action test/orfs/mock-array/results/asap7/Element/base/3_place.odb failed: (Exit 2): bash failed: error executing Action command (from target //test/orfs/mock-array:Element_place) /bin/bash -c ... (remaining 5 arguments skipped)

Notice `from target //test/orfs/mock-array:Element_place`.

From bottom you again find "error executing Action" and the relevant action:

    21:13:48  [ERROR GPL-0305] RePlAce diverged during gradient descent calculation, resulting in an invalid step length (Inf or NaN). This is often caused by numerical instability or high placement density. Consider reducing placement density to potentially resolve the issue.
    21:13:48  Error: global_place_skip_io.tcl, 12 GPL-0305
    21:13:48  Command exited with non-zero status 1
    21:13:48  Elapsed time: 0:40.32[h:]min:sec. CPU time: user 841.13 sys 49.36 (2208%). Peak memory: 157856KB.
    21:13:48  (21:13:48) [11,295 / 11,319] 777 / 782 tests, 1 failed; checking cached actions
    21:13:48  (21:13:48) ERROR: /tmp/workspace/OpenROAD-Public_PR-7814-head/test/orfs/mock-array/BUILD:116:10 Middleman _middlemen/test_Sorfs_Smock-array_Smake_UMockArray_Utest_Ubase_Utest-runfiles failed: (Exit 2): bash failed: error executing Action command (from target //test/orfs/mock-array:Element_place) /bin/bash -c ... (remaining 5 arguments skipped)
    21:13:48  

## Debugging an ORFS stage

Debugging using the Bazel `--sandbox_debug` is possible, but not terribly convenient. bazel-orfs has a debug feature specifically to debug stages and create standalone issues.

First set up a /tmp/place folder with the necessary dependencies:

    bazelisk run //test/orfs/mock-array:Element_place_deps /tmp/place

This sets up a `/tmp/place/make` script that is a small shell script that calls `make` on the ORFS setup in /tmp/place, but since the place stage failed, we have to build all place sub-stages up to the failing stage:

    /tmp/place/make do-place

Now create a standalone issue:

    /tmp/place/make global_place_skip_io_issue

The `WORK_HOME` is in `/tmp/place/_main`:

    $ ls /tmp/place/_main/
    ++ dirname /tmp/place/make
    + cd /tmp/place/_main
    [deleted]
    Archiving issue to global_place_skip_io_Element_asap7_base_2025-07-16_08-44.tar.gz
    Using pigz to compress tar file

## Using a local ORFS

bazel-orfs can set up ORFS design files locally for debugging purposes, leaving bazel-orfs entirely out of the equation when chasing down issues. Such a setup is most often a lot more convenient than using `--sandbox_debug`.

NOTE! keep in mind that these local ORFS design files have the depndencies `_deps` to run a particular stage only. Hence, use the `do-` prefix for doing `do-place`, `do-2_1_floorplan`, etc. so that `make` dependency checking is not used. If you use `make floorplan`, this will try to run synthesis first and not find the prequisite files, nor variables in config.mk, for synthesis and it will fail with bogus and confusing error messages.

If you're interested in some other stage, replace `place` with `synth`, `floorplan`, `cts`, `grt`, `route` or `final` below.

The `/tmp/place/make` script, if `FLOW_HOME` is set, will use a local ORFS and OpenROAD built by CMake:

    $ . ~/OpenROAD-flow-scripts/env.sh
    $ /tmp/place/make print-FLOW_HOME print-OPENROAD_EXE
    [deleted]
    FLOW_HOME = /home/<username>/OpenROAD-flow-scripts/flow
    OPENROAD_EXE = /home/<username>/OpenROAD-flow-scripts/tools/install/OpenROAD/bin/openroad

More explictly ORFS only:

    make --file=~/OpenROAD-flow-scripts/flow/Makefile -C /tmp/place/_main WORK_HOME=test/orfs/mock-array DESIGN_CONFIG=config.mk do-place

This is a bit more verbose, but eliminates any concerns about what the `/tmp/place/make` might be doing differently than ORFS only.

## Running a `make issue` with `cfg=exec` configuraiton

[TL;DR](../../docs/user/Bazel-targets.md), `bazelisk test ...` builds and uses the `cfg=exec` configuration when setting up paths:

    export PATH=/home/<username>/OpenROAD-flow-scripts/tools/OpenROAD/bazel-out/k8-opt-exec-ST-d57f47055a04/bin:$PATH
    $ ./run-me-Element-asap7-base.sh
    [deleted]
    [INFO GPL-1013] Final placement area: 94.65 (+0.00%)
    [ERROR GPL-0305] RePlAce diverged during gradient descent calculation, resulting in an invalid step length (Inf or NaN). This is often caused by numerical instability or high placement density. Consider reducing placement density to potentially resolve the issue.
    Error: global_place_skip_io.tcl, 12 GPL-0305

## Adding `tags = ["manual"]` and `test_kwargs = ["orfs"]` to BUILD files

In OpenROAD, `bazelisk build ...` should not build ORFS targets, only OpenROAD binaries.

Since bazel-orfs also has build targets, builds in Bazel can build anything, not just executables, the policy in OpenROAD is to mark non-binary build targets as `tags = ["manual"]`.

To hunt down missing `tags = ["manual"]` run a query like:

    bazelisk query 'kind(".*", //test/orfs/mock-array/...) except attr(tags, "manual", //test/orfs/mock-array/...)'

Note that OpenROAD *does* want `bazelisk test ...` to run all tests, so test targets should be marked `tags = ["orfs"]` instead, so that `.bazelrc` can skip builds of those targets with the `build --build_tag_filters=-orfs` line.
