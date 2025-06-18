# Testing local changes with Bazel

The main use-case for Bazel is to make modifications to OpenROAD and run local tests before creating a PR.

After [installing Baselisk](https://bazel.build/install/bazelisk), the command line below will discover all tests starting in the current working directory and build all dependencies, including openroad and opensta:

    bazelisk test ...

A word on expectations: Bazel is a valuable skill for the future. It is an example of a new generation of build tools(Buck2 is another example) that scales very well, are hermetic and have many features, such as artifacts. Unsurprisingly, this does mean that there is a lot to learn. The OpenROAD documentation makes no attempt at teaching Bazel. Bazel is a very wide topic and it can not be learned in a day or two with intense reading of a well defined document with a start and an end. It is probably best to start as a user running canned commands, such as above, but switch from mechanical repetition of canned command to being curious and following breadcrumbs of interest: read, search, engage with the community and use AI to learn.

## Running specific tests or tests below a folder

To list all tests:

    bazelisk query 'kind(test, ...)'

Run specific test:

    baselisk test //src/upf/test:levelshifter-tcl

To run or list tests below a folder:

    bazelisk test src/gpl/...

## Testing an OpenROAD build with ORFS from within the OpenROAD folder

    OPENROAD_EXE=$(pwd)/bazel-out/k8-opt-exec-ST-d57f47055a04/bin/openroad make --dir ~/OpenROAD-flow-scripts/flow/ DESIGN_CONFIG=designs/asap7/gcd/config.mk clean_floorplan floorplan

`$(pwd)/bazel-out/k8-opt-exec-ST-d57f47055a04/bin/openroad` points to the `cfg=exec` optimized configuration which is Bazel builds to run tests on host with e.g. `bazelisk test ...`

## Testing debug build of OpenROAD with ORFS

Build OpenROAD in debug mode using a [workaround](https://github.com/The-OpenROAD-Project/OpenROAD/issues/7349):

    bazelisk build --cxxopt=-stdlib=libstdc++ --linkopt=-lstdc++ -c dbg :openroad

Run ORFS flow and use debugger as usual for ORFS:

    OPENROAD_EXE=$(pwd)/bazel-out/k8-dbg/bin/openroad make --dir ~/OpenROAD-flow-scripts/flow/ DESIGN_CONFIG=designs/asap7/gcd/config.mk clean_floorplan floorplan

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
