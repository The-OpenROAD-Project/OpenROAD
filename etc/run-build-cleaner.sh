#!/usr/bin/env bash
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# Run build cleaner to optimized the deps = [] in cc_libraries.

set -u

readonly BANT_EXIT_ON_DWYU_ISSUES=3

BANT=$("$(dirname "$0")"/get-bant-path.sh)

# Run depend-on-what-you-use build-cleaner.
# Print buildifier commands to fix if needed.
"${BANT}" dwyu --graph-augment="..." "$@"

BANT_EXIT=$?
if [ ${BANT_EXIT} -eq ${BANT_EXIT_ON_DWYU_ISSUES} ]; then
  cat >&2 <<EOF
------------------------------------------------------------------
Build dependency issues found, the following one-liner will fix it.

source <(etc/run-build-cleaner.sh $@)
EOF
fi

exit $BANT_EXIT
