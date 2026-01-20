# ORFS smoke tests

A set of ORFS integration tests that runs in a few minutes suitable for inclusion in the local fast regression testing workflow prior to creating a PR:

    bazelisk test ...

## Updating RULES_JSON files

1. Run `bazelisk run //test/orfs/gcd:gcd_update` to update RULES_JSON file for a design. This will build and run OpenROAD to generate a new RULES_JSON file and update the RULES_JSON source file.
2. Create commit for RULES_JSON file

## Updating all RULES_JSON files

Oneliner that runs tests, which builds all prerequisites in parallel, then update all the rules:

    bazelisk test test/orfs/... && bazelisk query test/orfs/... | grep _update\$ | xargs -n1 bazelisk run

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

## eqy tests

`eqy_test` is used to run equivalence checks before and after an ORFS stage, such as before and after floorplan for mock-array. To run the test and keep all the files from the test and see interactive output, run:

    bazelisk test //test/orfs/mock-array:MockArray_4x4_eqy_test --test_output=streamed --sandbox_debug

If this fails, then it will output the line below. `eqy` uses a very, very large number of files and copying out these files to the bazel-testlogs folder for inspection takes some time:

    Copying 114462 files to bazel-testlogs/test/orfs/mock-array/MockArray_4x4_eqy_test/test.outputs for inspection.

If you just want the files needed to run a locally installed `eqy`, build all the files used in the run above by:

    bazelisk build //test/orfs/mock-array:MockArray_4x4_eqy_test

The files used to run the test are then in `bazel-bin/test/orfs/mock-array/`.

### TL;DR creating archive for standalone eqy test

Create `/tmp/issue.tar.gz`

    bazelisk build //test/orfs/gcd:gcd_eqy_synth_test
    tar --exclude='*oss_cad_suite*' -czvf /tmp/issue.tar.gz -C "$(bazelisk info bazel-bin)/test/orfs/gcd/gcd_eqy_synth_test.run.sh.runfiles" .

Reproduction instructions:

1. untar archive
2. cd _main
3. ~/oss-cad-suite/bin/eqy test/orfs/gcd/gcd_eqy_synth_test.eqy

Comments:

- `~/oss-cad-suite/` should be familiar with anyone versed in eqy as it refers to the [official binary release binaries](https://github.com/YosysHQ/oss-cad-suite-build)
- Reproduction cases should be minimal in terms of size and context. The above recipe contains some unnecessary files, unnecessary directory structure, etc. Prune the test case further manually to get to the core of the matter.

## Running eqy tests standalone outside bazel

Build the test files:

    bazelisk build //test/orfs/gcd:gcd_eqy_synth_test

This outputs:

    Target //test/orfs/gcd:gcd_eqy_synth_test up-to-date:
      bazel-bin/test/orfs/gcd/gcd_eqy_synth_test.run.sh

Copy all files needed to run the tests into a working folder `foo`:

    rsync -aL "$(bazelisk info bazel-bin)/test/orfs/gcd/gcd_eqy_synth_test.run.sh.runfiles/" foo/

Change cwd for running the tests:

    cd foo/_main

Bazel sets up both binaries and data files to run the tests, to create a reportable standalone test. A report should include only the data files and not the actual binaries used locally to run the test:

    $ find .
    .
    ./test
    ./test/orfs
    ./test/orfs/asap7
    ./test/orfs/asap7/asap7sc7p5t_SIMPLE_RVT_TT_201020.v
    ./test/orfs/asap7/asap7sc7p5t_INVBUF_RVT_TT_201020.v
    ./test/orfs/asap7/asap7sc7p5t_AO_RVT_TT_201020.v
    ./test/orfs/asap7/asap7sc7p5t_OA_RVT_TT_201020.v
    ./test/orfs/gcd
    ./test/orfs/gcd/gcd.v
    ./test/orfs/gcd/gcd_eqy_synth_test.run.sh
    ./test/orfs/gcd/gcd_eqy_synth.v
    ./test/orfs/gcd/gcd_eqy_synth_test.eqy

Bazel generated `bazel-bin/test/orfs/gcd/gcd_eqy_synth_test.run.sh` to runs the tests. Examine this script to look at that for clues as to how to run the tests standalone:

    $ head -n 5 test/orfs/gcd/gcd_eqy_synth_test.run.sh 
    # !/bin/sh
    set -euo pipefail
    test_status=0
    (exec ../bazel-orfs++_repo_rules+oss_cad_suite/bin/eqy "$@" test/orfs/gcd/gcd_eqy_synth_test.eqy) || test_status=$?

What we want is:

    ../bazel-orfs++_repo_rules+oss_cad_suite/bin/eqy test/orfs/gcd/gcd_eqy_synth_test.eqy

Running this, we get:

<pre>$ ../bazel-orfs++_repo_rules+oss_cad_suite/bin/eqy test/orfs/gcd/gcd_eqy_synth_test.eqy
<font color="#12488B">EQY</font> <font color="#26A269"> 8:05:24</font> [<font color="#12488B">gcd_eqy_synth_test</font>] <font color="#A347BA">read_gold</font>: starting process &quot;yosys -ql gcd_eqy_synth_test/gold.log gcd_eqy_synth_test/gold.ys&quot;
<font color="#12488B">EQY</font> <font color="#26A269"> 8:05:24</font> [<font color="#12488B">gcd_eqy_synth_test</font>] <font color="#A347BA">read_gold</font>: finished (returncode=0)
<font color="#12488B">EQY</font> <font color="#26A269"> 8:05:24</font> [<font color="#12488B">gcd_eqy_synth_test</font>] <font color="#A347BA">read_gate</font>: starting process &quot;yosys -ql gcd_eqy_synth_test/gate.log gcd_eqy_synth_test/gate.ys&quot;
<font color="#12488B">EQY</font> <font color="#26A269"> 8:05:25</font> [<font color="#12488B">gcd_eqy_synth_test</font>] <font color="#A347BA">read_gate</font>: finished (returncode=0)
<font color="#12488B">EQY</font> <font color="#26A269"> 8:05:25</font> [<font color="#12488B">gcd_eqy_synth_test</font>] <font color="#A347BA">combine</font>: starting process &quot;yosys -ql gcd_eqy_synth_test/combine.log gcd_eqy_synth_test/combine.ys&quot;
<font color="#12488B">EQY</font> <font color="#26A269"> 8:05:25</font> [<font color="#12488B">gcd_eqy_synth_test</font>] <font color="#A347BA">combine</font>: finished (returncode=0)
<font color="#12488B">EQY</font> <font color="#26A269"> 8:05:25</font> [<font color="#12488B">gcd_eqy_synth_test</font>] <font color="#A2734C"><b>Warning: Cannot find entity _*_.*.</b></font>
[deleted]
<font color="#12488B">EQY</font> <font color="#26A269"> 8:05:32</font> [<font color="#12488B">gcd_eqy_synth_test</font>] <font color="#C01C28"><b>Failed to prove equivalence of partition gcd.resp_msg.1</b></font>
[deleted]
<font color="#12488B">EQY</font> <font color="#26A269"> 8:05:32</font> [<font color="#12488B">gcd_eqy_synth_test</font>] DONE (FAIL, rc=2)
</pre>


## using sv-bugpoint to whittle down test-cases

[sv-bugpoint](https://github.com/antmicro/sv-bugpoint) can be used to whittle down test-cases to a minimum example.

Let's say that `bazelisk test test/orfs/gcd:eqy_synth_test` fails, first create a script that looks for the error string in the output, in this case a false positive `Failed to prove equivalence of partition gcd.req_rdy`.

After compiling sv-bugpoint, we create a check.sh script:

```bash
#!/bin/bash
set -euo pipefail
cp $1 test/orfs/gcd/
(bazelisk test test/orfs/gcd:eqy_synth_test --test_timeout=30 --test_output=streamed || error=$?) | tee /dev/tty | grep "Failed to prove equivalence of partition gcd.req_rdy"
```

- `--test_timeout=30` is a suitable timeout for this test and machine, adjust. Note that the timeout must be handled by Bazel and not a generic `timeout` utility as it would not work correctly with the bazel server.


Next, run sv-bugpoint to whittle `test/orfs/gcd/gcd.v` down to a minimal test case:

    ~/sv-bugpoint/build/sv-bugpoint fail check.sh test/orfs/gcd/gcd.v
