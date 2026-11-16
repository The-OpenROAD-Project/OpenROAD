#!/usr/bin/env bash
# Outputs key-value pairs consumed by Bazel workspace stamping.
# Keys prefixed with STABLE_ cause downstream rebuilds when their value changes.
# Bazel runs this on every build, with or without --stamp. Only the version
# copy of //:openroad (bazel/stamped_binary.bzl) and stamped man pages read it.

# OPENROAD_VERSION sets the version explicitly. Use it when the tree has no
# git metadata, for example in a Docker build, because .dockerignore keeps
# .git out of the build context.
# Bazel gives this command the environment of the client, but each server
# caches that environment. Set the value before the first build, or run
# `bazelisk clean` after you change it.
if [[ -n "${OPENROAD_VERSION:-}" ]]; then
  GIT_VERSION="${OPENROAD_VERSION}"
else
  # --dirty marks a build of uncommitted changes, including a changed
  # submodule such as openroad/sta.
  GIT_VERSION=$(git describe --tags --match '[0-9][0-9]Q[0-9]' --always --dirty \
    2>/dev/null || echo "unknown")
fi
echo "STABLE_GIT_VERSION ${GIT_VERSION}"

# Committer time of HEAD as a Unix timestamp, the SOURCE_DATE_EPOCH format.
# //docs:man_pages stamps it into each page's footer date.
GIT_COMMIT_TIME=$(git log -1 --format=%ct 2>/dev/null || echo "")
echo "STABLE_GIT_COMMIT_TIME ${GIT_COMMIT_TIME}"

STA_GIT_SHA1=$(git -C openroad/sta rev-parse HEAD 2>/dev/null || echo "unknown")
echo "STABLE_STA_GIT_SHA1 ${STA_GIT_SHA1}"
