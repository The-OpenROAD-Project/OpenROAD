# Testing local changes with Bazel

The main use-case for Bazel is to make modifications to OpenROAD and run local tests before creating a PR.

After [installing Baselisk](https://bazel.build/install/bazelisk), the command line below will discover all tests starting in the current working directory and build all dependencies, including openroad:

    bazelisk test -c opt ...

A word on expectations: Bazel is a valuable skill for the future. Is a example of a new generation of build tools(Buck2 is another example) that scales very well, is hermetic and has many features, such as artifacts. Unsurprisingly, this does mean that there is a lot to learn. The OpenROAD documentation makes no attempt at teaching Bazel. Bazel is a very wide topic and it can not be learned in a day or two with intense reading of a well defined document with a start and an end. It is probably best to start as a user running canned commands, such as above, but switch from mechanical repetition of canned command to being curious and following breadcrumbs of interest: read, search, engage with the community and use AI to learn.

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
