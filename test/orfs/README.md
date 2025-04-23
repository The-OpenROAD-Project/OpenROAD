# ORFS smoke tests

A set of ORFS integration tests that runs in a few minutes suitable for inclusion in the local fast regression testing workflow prior to creating a PR:

    bazel -c opt test ...

## Updating ORFS and bazel-orfs

`bazel run @bazel-orfs//:bump`, this will find the latest bazel-orfs and ORFS docker image and update MODULE.bazel and MODULE.bazel.lock.
