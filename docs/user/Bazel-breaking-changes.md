# Handling breaking changes between OpenROAD and ORFS

ORFS occasionally requires an update when OpenROAD has changed in some breaking way(such as when retiring a feature), but it is not common. 

Normally a new feature is added to OpenROAD, then ORFS afterwards can be updated to use the new feature, or if ORFS has a breaking change for bazel-orfs, ORFS and bazel-orfs can be updated together in OpenROAD.

## Add interim conditional code to ORFS

One way to handle this is to add conditional code to ORFS and clean up the conditional code when it is no longer needed.

```tcl
try {
    new_command -option value
} trap {ANY} {e} {
    puts "new_command failed, falling back to old_command"
    old_command -option value
}
```

## Publish a Docker image with the changes

To update MODULE.bazel to point to a new ORFS docker image and bazel-orfs in OpenROAD, run [bazelisk run //bazel-orfs:bump](../../test/orfs/README.md#updating-orfs-and-bazel-orfs), then use `git add -p .` to pick what you want to commit.

Then, to create an OpenROAD PR which uses an interim ORFS Docker image:

1. publish a docker image of ORFS with the changes and the old broken OpenROAD, but don't update ORFS master. OpenROAD uses the ORFS docker image, with the bazel compiled OpenROAD
2. point MODULE.bazel to this docker image
3. create a PR for OpenROAD. This PR will run with HEAD from OpenROAD and the ORFS image you published in 1.
4. Upgrade the OpenROAD submodule in ORFS and publish a new Docker image
5. Run `bazelisk run //bazel-orfs:bump` again
