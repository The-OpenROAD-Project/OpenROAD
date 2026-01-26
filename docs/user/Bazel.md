# Testing local changes with Bazel

First [install Baselisk](https://bazel.build/install/bazelisk), then you're ready for the main use-case of Bazel, which is to make modifications to OpenROAD and run fast local tests before creating a PR:

    bazelisk test --jobs=4 src/...

- `...` means everything below this folder, so use `src/gpl/...` to run a smaller set of tests.
- `--jobs=4` limits parallel builds to 4 cores, default is use all cores.

For more comprehensive testing locally, includes longer OpenROAD integration tests and some ORFS smoke tests, install either [podman](https://podman.io/), which works without root permissions or Docker, then run:

    bazelisk test ...

Note! You'll see `bazel` in examples and documentation as well as `bazelisk`. The latter is a wafer thin layer on top of `bazel` that reads in the `.bazelversion` file to decide which version of Bazel to use in bazel.

A word on expectations: Bazel is a valuable skill for the future. It is an example of a new generation of build tools(Buck2 is another example) that scales very well, are hermetic and have many features, such as artifacts. Unsurprisingly, this does mean that there is a lot to learn. The OpenROAD documentation makes no attempt at teaching Bazel. Bazel is a very wide topic and it can not be learned in a day or two with intense reading of a well defined document with a start and an end. It is probably best to start as a user running canned commands, such as above, but switch from mechanical repetition of canned command to being curious and following breadcrumbs of interest: read, search, engage with the community and use AI to learn.

## Running specific tests or tests below a folder

To list all tests use bazelisk query with [Bazel query language](https://bazel.build/query/language):

    bazelisk query 'kind(test, ...)'

List all tests below a folder containing `asap7`:

    $ bazelisk query   'filter("asap7", kind(".*_test rule", //src/pdn/...))'
    //src/pdn/test:asap7_M1_M3_followpins-tcl
    ...

Run specific test:

    baselisk test --test_output=errors //src/upf/test:levelshifter-tcl

To run or list tests below a folder:

    bazelisk test src/gpl/...

## Build configurations

Bazel build configuration is a big topic as it covers cross compilation as well as multiplatform support.

As a start, browse `.bazelrc` and also run:

    $ bazelisk build
    [wait until it starts building then hist ctrl-c, this warms up the cache with information about available configs]
    $ bazelisk config
    Available configurations:
    5e5e8a80a777eb91e67b2d19d33945262b2897b636da1007246cf68b6b6ec51d k8-fastbuild
    781deb76199c2d7f1a6c7d54da3ea8dad03e6956c04db58addba49068e4d3797 k8-fastbuild
    96deab75888eab9e42bcc5778aa824b18bb8ee745dfa1950a18536d75ee05e01 k8-opt-exec-ST-d57f47055a04 (exec)
    dd6f5325b352d3c63abe45b22d15914f75da7e2f140ace0017440dfc00f49114 k8-opt-exec-ST-d57f47055a04 (exec)
    f37096aa0a6acd138beca4ff4d66b677012cd1d0d54befaa35983993505dad60 fastbuild-noconfig
    f6ad5d5ec52c67510c6642c6d8df81fb134760a95c43d1f8e0369ad2c8964a81 k8-opt-exec-ST-6f5a6fb95be7 (exec)

Without offering any deeper insight some comments about what is shown above:

- Each configuration has a textual shorthand used in folder names followed by a full hash
- `fastbuild` is the default compromise between optimization and fast builds
- `exec` means host, appears with `ST` and an extra hash at the end
- `k8` always there, possibly referring to [K8](https://en.wikipedia.org/wiki/X86-64)

## Build without testing

    bazelisk build :openroad

## Platforms

https://bazel.build/extending/platforms

Note that this builds a different configuration than is used during tests. Tests run with the `cfg=exec` configuraiton, whereas the above builds the `cfg=target` configuration. The TL;DR is that you are probably better of running a single test to test building so that you don't have to rebuild if you want to run tests after testing build.

## Can I force a rebuild?

You should never have to in Bazel as builds are robust, trust the caching...

That said:

    bazelisk clean

Or more forcefully:

    bazelisk clean --expunge

Or "nuke it from orbit":

    baselisk shutdown
    sudo pkill -9 java
    sudo rm -rf ~/.cache/bazel

## Run tests with [address sanetizers](https://github.com/google/sanitizers/wiki/addresssanitizer):

    bazelisk test --config=asan src/...

Example output:

```
[deleted]
Direct leak of 18191 byte(s) in 2525 object(s) allocated from:
    #0 0x5f8556654e04  (/home/oyvind/.cache/bazel/_bazel_oyvind/896cc02f64446168f604c13ad7b60f8b/execroot/_main/bazel-out/k8-opt-exec-ST-d57f47055a04/bin/external/org_swig/swig+0x37de04) (BuildId: f982b51b51338154ba961612c62b330f)
[deleted]
SUMMARY: AddressSanitizer: 27236 byte(s) leaked in 3801 allocation(s).
[deleted]
```

## Testing an OpenROAD build with ORFS from within the OpenROAD folder

    OPENROAD_EXE=$(pwd)/bazel-out/k8-opt-exec-ST-d57f47055a04/bin/openroad make --dir ~/OpenROAD-flow-scripts/flow/ DESIGN_CONFIG=designs/asap7/gcd/config.mk clean_floorplan floorplan

`$(pwd)/bazel-out/k8-opt-exec-ST-d57f47055a04/bin/openroad` points to the `cfg=exec` optimized configuration which is Bazel builds to run tests on host with e.g. `bazelisk test ...`

## Testing debug build of OpenROAD with ORFS

Build OpenROAD in debug mode using a [workaround](https://github.com/The-OpenROAD-Project/OpenROAD/issues/7349):

    bazelisk build --cxxopt=-stdlib=libstdc++ --linkopt=-lstdc++ -c dbg :openroad

Run ORFS flow and use debugger as usual for ORFS:

    OPENROAD_EXE=$(pwd)/bazel-out/k8-dbg/bin/openroad make --dir ~/OpenROAD-flow-scripts/flow/ DESIGN_CONFIG=designs/asap7/gcd/config.mk clean_floorplan floorplan

## Profiling OpenROAD with an ORFS build and your favorite profiling tool

Build an optimized profile binary, using a [workaround](https://github.com/The-OpenROAD-Project/OpenROAD/issues/7349):

    bazelisk build --config=profile --cxxopt=-stdlib=libstdc++ --linkopt=-lstdc++ :openroad

`bazel-bin` points to the results of the most recent `bazelisk build`. If you are switching between various builds, the more robust alternative is to point `OPENROAD_EXE` to the specific build configuration you want in `bazel-out`.

Start an ORFS job that you want to profile:

    OPENROAD_EXE=$(pwd)/bazel-bin/openroad make --dir ~/OpenROAD-flow-scripts/flow/ DESIGN_CONFIG=designs/asap7/gcd/config.mk clean_floorplan floorplan

At this point, use your favorite preformance tool, such as the Linx Perf tool:

    perf top

Perhaps attach gdb and use ctrl-c from the command line? Use gdb with an IDE, emacs, or vim?

    $ gdb bazel-bin/openroad
    [deleted]
    (gdb) attach 578603
    Attaching to program: /home/oyvind/.cache/bazel/_bazel_oyvind/896cc02f64446168f604c13ad7b60f8b/execroot/_main/bazel-out/k8-fastbuild/bin/openroad, process 578603
    #7  0x00005efc3b72b865 in isPolygonCorner () at src/drt/src/gc/FlexGC_init.cpp:705
    705	  poly_set.get(polygons);
    (gdb) list
    700	bool isPolygonCorner(const frCoord x,
    701	                     const frCoord y,
    702	                     const gtl::polygon_90_set_data<frCoord>& poly_set)
    703	{
    704	  std::vector<gtl::polygon_90_with_holes_data<frCoord>> polygons;
    705	  poly_set.get(polygons);
    706	  for (const auto& polygon : polygons) {
    707	    for (const auto& pt : polygon) {
    708	      if (pt.x() == x && pt.y() == y) {
    709	        return true;

## Creating an ORFS issue with bazel-orfs targets using `_deps` targes

Consider a failure in `//test/orfs/mock-array:MockArray_floorplan` as one can find if carefully searching the logs for `ERROR:` and looking for `target`:

    $ bazelisk test //test/orfs/mock-array:MockArray_test
    [deleted]
    ERROR: /tmp/workspace/OpenROAD-Public_PR-7619-head/test/orfs/mock-array/BUILD:116:10: Action test/orfs/mock-array/results/asap7/MockArray/base/2_floorplan.odb failed: (Exit 2): bash failed: error executing Action command (from target //test/orfs/mock-array:MockArray_floorplan) /bin/bash -c ... (remaining 5 arguments skipped)
    [deleted]
    [ERROR MPL-0040] Failed on cluster root
    Error: macro_place.tcl, 5 MPL-0040
    [deleted]
    //test/orfs/mock-array:MockArray_test                           FAILED TO BUILD

To create an ORFS `make issue`, follow these steps:

    rm -rf /tmp/floorplan
    bazelisk run //test/orfs/mock-array:MockArray_floorplan_deps /tmp/floorplan

- In Bazel `//test/orfs/mock-array:MockArray_floorplan` failed and will leave behind no files, unless one uses `--sandbox_debug`
- bazel-orfs adds a `//test/orfs/mock-array:MockArray_floorplan_deps` target that sets up all the dependencies for running `make do-floorplan`, similarly for `_synth/place/cts/grt/route/final`.
- `tmp/floorplan` folder that contains a `make` script that is very nearly the same as `make DESIGN_CONFIG=...` with ORFS

First run `do-floorplan` until the failure, notice that the `do-` prefix is used to disable the dependency checking in ORFS as bazel-orfs handles dependencies:

    /tmp/floorplan/make do-floorplan

Now create an issue for e.g. `macro_place.tcl`:

    $ /tmp/floorplan/make macro_place_issue
    ++ dirname /tmp/floorplan/make
    + cd /tmp/floorplan/_main
    + exec ./make_MockArray_floorplan_base_2_floorplan floorplan_issue
    [deleted]

The generated file is placed into /tmp/floorplan/_main:

    /tmp/floorplan/_main/macro_place_MockArray_asap7_base_2025-06-19_21-50.tar.gz

## Creating an ORFS issue with bazel-orfs targets using `--sandbox_debug`

Hermeticity in Bazel requires some extra steps when debugging failures. If the action fails, then `--sandbox_debug` can be used. If the action succeeds or it is cached, `--sandbox_debug` does nothing.

If you have a failure in `//test/orfs/mock-array:MockArray_floorplan`, look for `ERROR:` and looking for `target`, find the error:

    $ bazelisk test //test/orfs/mock-array:MockArray_test
    [deleted]
    ERROR: /tmp/workspace/OpenROAD-Public_PR-7619-head/test/orfs/mock-array/BUILD:116:10: Action test/orfs/mock-array/results/asap7/MockArray/base/2_floorplan.odb failed: (Exit 2): bash failed: error executing Action command (from target //test/orfs/mock-array:MockArray_floorplan) /bin/bash -c ... (remaining 5 arguments skipped)
    [deleted]
    [ERROR MPL-0040] Failed on cluster root
    Error: macro_place.tcl, 5 MPL-0040
    [deleted]
    //test/orfs/mock-array:MockArray_test                           FAILED TO BUILD

Use `--sandbox_debug` to keep the files around after failure:

    bazelisk build //test/orfs/mock-array:MockArray_floorplan --sandbox_debug

Scan the log for setting up the shell and enviornment variables without linux-sandbox.

    (cd /home/oyvind/.cache/bazel/_bazel_oyvind/896cc02f64446168f604c13ad7b60f8b/sandbox/linux-sandbox/8901/execroot/_main && \
    exec env - \
        DESIGN_CONFIG=bazel-out/k8-fastbuild/bin/test/orfs/mock-array/results/asap7/MockArray/base/2_floorplan.mk \
        [deleted]
    /home/oyvind/.cache/bazel/_bazel_oyvind/install/772f324362dbeab9bc869b8fb3248094/linux-sandbox -t 15 -w /dev/shm -w /home/oyvind/.cache/bazel/_bazel_oyvind/
    [deleted]
    /mock-array/reports/asap7/MockArray/base/2_floorplan_final.rpt && external/bazel-orfs++orfs_repositories+docker_orfs/usr/bin/make $@' '' --file external/bazel-orfs++orfs_repositories+docker_orfs/OpenROAD-flow-scripts/flow/Makefile do-floorplan)

Do a bit of suregery to remove the `linux-sandbox` and `exec env -` part, which can be a bit tempremental, to launch a bash shell. This leaves you with a) changing directory b) setting up environment variables c) launching bash shell:

    (cd /home/oyvind/.cache/bazel/_bazel_oyvind/896cc02f64446168f604c13ad7b60f8b/sandbox/linux-sandbox/8901/execroot/_main && \
        DESIGN_CONFIG=bazel-out/k8-fastbuild/bin/test/orfs/mock-array/results/asap7/MockArray/base/2_floorplan.mk \
        [deleted]
    bash)

Now run `make issue` as usual:

    $ make --file external/bazel-orfs++orfs_repositories+docker_orfs/OpenROAD-flow-scripts/flow/Makefile macro_place_issue
    Archiving issue to macro_place_MockArray_asap7_base_2025-06-20_12-57.tar.gz
    Using pigz to compress tar file

## Some OpenROAD and OpenSTA Bazel Specifics

Bazel distinguishes between *host* (`cfg=exec`) and *target* (`cfg=target`) configurations, a concept that becomes important when cross-compilation or tool usage is involved.

In the OpenROAD Bazel build:

- `bazelisk build ...` builds all targets in the **target configuration** (`cfg=target`), assuming you're building for deployment or installation.
- `bazelisk test ...`, on the other hand, uses OpenROAD and OpenSTA **as host tools**, meaning they are built and run in the **execution configuration** (`cfg=exec`), often to run tests or launch `bazel-orfs` builds.

### ⚠️ Avoiding Redundant Builds

By default, `bazel test` would:
1. First build test dependencies in the **target** configuration.
2. Then build tools like OpenROAD/OpenSTA again in the **host** configuration to actually run the tests.

This causes unnecessary duplication.

To avoid this, `.bazelrc` includes the following to build only the tests:

    test --build_tests_only

## Using the OpenROAD project Bazel artifact server to download pre-built results

A single read only artifact server is configured in `.bazelrc` OpenROAD hosted projects.

This is a read only Bazel artifact server for anonymous access and is normally only updated by OpenROAD CI, though team OpenROAD team members can also update it directly.

## OpenROAD team member and CI - configuring write access to artifact server

If you only have a single Google account that you use for Google Cloud locally, you can use
`--google_default_credentials`.

If you are use multiple google accounts, using the default credentials can be cumbersome when
switching between projects. To avoid this, you can use the `--credential_helper` option
instead, and pass a script that fetches credentials for the account you want to use. This
account needs to have logged in using `gcloud auth login` and have access to the bucket
specified.

`.bazelrc` is under git version control and it will try to read in [user.bazelrc](https://bazel.build/configure/best-practices#bazelrc-file), which is
not under git version control, which means that for git checkout or rebase operations will
ignore the user configuration in `user.bazelrc`.

Copy the snippet below into `user.bazelrc` and specify your username by modifying `# user: myname@openroad.tools`:

    # user: myname@openroad.tools
    build --credential_helper=*.googleapis.com=%workspace%/etc/cred_helper.py

`cred_helper.py` will parse `user.bazelrc` and look for the username in the comment.

To test, run:

    $ ./cred_helper.py test
    Running: gcloud auth print-access-token oyvind@openroad.tools
    {
      "kind": "storage#testIamPermissionsResponse",
      "permissions": [
        "storage.buckets.get",
        "storage.objects.create"
      ]
    }

> **Note:** To test the credential helper, make sure to restart Bazel to avoid using a previous
cached authorization:

    bazel shutdown
    bazel build BoomTile_final_scripts

To gain write access to the https://storage.googleapis.com/megaboom-bazel-artifacts bucket,
reach out to Tom Spyrou, Precision Innovations (https://www.linkedin.com/in/tomspyrou/).

## Bisecting OpenSTA or OpenROAD with Bazel

Bisecting OpenROAD or OpenSTA requires finding a good and a bad commit. Normally in bisection, origin/master is bad, but finding a good commit is trickier because most commits are there solely to preserve review history and have not run through any extensive testing.

Fortunately, OpenSTA is a submodule in OpenROAD that is tested before it is updated, so all the submodule commits in OpenROAD of OpenSTA are known to be of good quality. Similarly for OpenROAD and ORFS.

A git/bash incantation will list the commit hashes of the src/sta submodule:

    $ git log --pretty=format:'%h' -- src/sta | while read commit; do  git show $commit src/sta| grep "Subproject commit" | awk '{print $3}'; done | head -n 10
    5ee1a315141d1c799a0b2532e90ddccf52ddee95
    3bff2d218c20adb867fcb3d8ae236f5da9928bed
    3bff2d218c20adb867fcb3d8ae236f5da9928bed
    5ee1a315141d1c799a0b2532e90ddccf52ddee95
    f21d4a3878e2531e3af4930818d9b5968aad9416
    3bff2d218c20adb867fcb3d8ae236f5da9928bed
    522fc9563f25728f456bf86c2eb665c60d823e74
    f21d4a3878e2531e3af4930818d9b5968aad9416
    fa0cdd65290843e4e5cbe39d0bb9f2a63d580d1f
    522fc9563f25728f456bf86c2eb665c60d823e74

To build OpenSTA, use master of the https://github.com/The-OpenROAD-Project/OpenSTA fork, because it contains Bazel build files:

    bazelisk build src/sta:opensta -c opt

Now start the bisection as usual with a bad and good commit from the above list:

    $ cd src/sta
    $ git bisect start origin/master 6e95d93a44f7c46bb572933f5e2f8a624135820b
    HEAD is now at 6e95d93a Merge remote-tracking branch 'parallax/master'
    Bisecting: 55 revisions left to test after this (roughly 6 steps)
    [03d2a48f462105a39b5850b8f45d6c5db16fd5f0] misc

Use `git bisect --skip` if the version does not build or otherwise should not be tested.

OpenSTA has an additional challenge in that only the https://github.com/The-OpenROAD-Project/OpenSTA fork has the Bazel BUILD file. To bisect the https://github.com/parallaxsw/OpenSTA branch, check out the branch you want, then check out BUILD from the fork and do a `git reset HEAD`. This will leave BUILD as a local file, because it is not in the upstream repository and bisection can be done on the upstream master branch.

## Testing the GUI with gcd on a pull request by number

To test a PR with the GUI on gcd, run:

```
    $ git fetch origin pull/7856/head
    $ git checkout FETCH_HEAD
    $ bazelisk run test/orfs/gcd:gcd_final /tmp/gcd -- gui_final
```

This will:

- fetch and checkout pull request 7856
- build OpenROAD
- run bazel-orfs flow on gcd
- create a /tmp/gcd folder with the ORFS project
- launch the GUI opening gui_final gcd

`bazelisk run test/orfs/gcd:gcd_final` run alone would create the `/tmp/gcd` folder and the arguments. The arguments after `--` are forwarded to the `/tmp/gcd/make` script that invokes make with the gcd ORFS project set up in `/tmp/gcd/_main/config.mk`.

## Hacking ORFS with `//test/orfs/gcd:gcd_test` test case

First create a local work folder with all dependencies for the step that you want to work on:

    bazelisk run //test/orfs/gcd:gcd_floorplan_deps /tmp/floorplan

Now run make directly with the `/tmp/floorplan/_main` work folder, but be sure to use the `do-` targets that side-step ORFS make dependency checking:

    make --file ~/OpenROAD-flow-scripts/flow/Makefile --dir /tmp/floorplan/_main DESIGN_CONFIG=config.mk do-floorplan

## Whittling down .odb files with deltaDebug.py

Global place can take hours to run and to debug an error, the test case has to be whittled down to minutes, or it is probably intractable.

Consider an error such as:

    [ERROR GPL-0305] RePlAce diverged during gradient descent calculation, resulting in an invalid step length (Inf or NaN). This is often caused by numerical instability or high placement density. Consider reducing placement density to potentially resolve the issue.

First create a folder with all the dependencies to run global placement:

    bazelisk run //test/orfs/gcd:gcd_deps /tmp/bug

Drop into a shell that has the build environment set up:

    $ /tmp/bug/make bash
    Makefile Environment  /tmp/bug/_main

Run up to the failing stage and stop with ctrl-c on the step that you want to run the whittling down on:

    make --file=$FLOW_HOME/Makefile do-place

Now run deltaDebug.py:

    $OPENROAD_EXE -python ~/OpenROAD-flow-scripts/tools/OpenROAD/etc/deltaDebug.py --error_string GPL-0305 --base_db_path test/orfs/gcd/results/asap7/gcd/base/3_2_place_iop.odb --use_stdout --exit_early_on_error --step "make --file=$FLOW_HOME/Makefile do-3_3_place_gp"

This should eventually leave you with a whittled down .odb file. Copy the whittled down .odb file into the correct place for 3_2_place_iop.odb, then create a bug report:

    /tmp/bug/make global_place_issue
