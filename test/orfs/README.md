# ORFS smoke tests

A set of ORFS integration tests that runs in a few minutes suitable for inclusion in the local fast regression testing workflow prior to creating a PR:

    bazelisk test ...

## Updating RULES_JSON files

1. Run `bazelisk run //test/orfs/gcd:gcd_update` to update RULES_JSON file for a design. This will build and run OpenROAD to generate a new RULES_JSON file and update the RULES_JSON source file.
2. Create commit for RULES_JSON file

## Updating ORFS and bazel-orfs

`bazelisk run @bazel-orfs//:bump`, this will find the latest bazel-orfs and ORFS docker image and update MODULE.bazel and MODULE.bazel.lock.
