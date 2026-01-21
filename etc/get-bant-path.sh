#!/usr/bin/env bash

set -eu

# Print path to a bant binary. Can be provided by an environment variable
# or built from our dependency.

BANT=${BANT:-needs-to-be-compiled-locally}

# Bant not given, compile from bzlmod dep.
if [ "${BANT}" = "needs-to-be-compiled-locally" ]; then
  BAZEL=${BAZEL:-bazelisk}
  if ! command -v "${BAZEL}">/dev/null ; then
    BAZEL=bazel
  fi
  # run_under will print the final path.
  BANT="$("${BAZEL}" run -c opt --run_under='echo' @bant//bant:bant 2>/dev/null)"
fi

echo "$BANT"
