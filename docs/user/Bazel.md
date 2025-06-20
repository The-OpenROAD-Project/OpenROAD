# Testing local changes with Bazel

The main use-case for Bazel is to make modifications to OpenROAD and run local tests before creating a PR.

After [installing Baselisk](https://bazel.build/install/bazelisk), the command line below will discover all tests starting in the current working directory and build all dependencies, including openroad and opensta:

    bazelisk test --jobs=4 ...

- `...` means everything below this folder, so use `src/gpl/...` to run a smaller set of tests.
- `--jobs=4` limits parallel builds to 4 cores, default is use all cores.

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
