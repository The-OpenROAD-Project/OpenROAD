# Building with Bazel


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
    build --credential_helper=%workspace%/etc/cred_helper.py

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
