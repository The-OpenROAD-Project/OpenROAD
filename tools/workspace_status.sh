#!/usr/bin/env bash
# Outputs key-value pairs consumed by Bazel workspace stamping.
# Keys prefixed with STABLE_ cause downstream rebuilds when their value changes.
# With --nostamp (default), Bazel substitutes empty strings, keeping the cache intact.

# OPENROAD_VERSION sets the version explicitly, as -DOPENROAD_VERSION does for
# CMake. Use it when the tree has no git metadata, for example in a Docker
# build, because .dockerignore keeps .git out of the build context.
# Bazel gives this command the environment of the client, but each server
# caches that environment. Set the value before the first build, or run
# `bazel clean` after you change it.
if [[ -n "${OPENROAD_VERSION:-}" ]]; then
  GIT_VERSION="${OPENROAD_VERSION}"
else
  GIT_VERSION=$(git describe --tags --match '[0-9][0-9]Q[0-9]' --always 2>/dev/null \
    || echo "unknown")
fi
echo "STABLE_GIT_VERSION ${GIT_VERSION}"
