#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Run each Tcl script with the standalone odbtcl shell.
# Usage: odbtcl_test.sh ODBTCL SCRIPT...
# A script reports failure with a non-zero exit status.
set -euo pipefail

odbtcl=$1
shift
for script in "$@"; do
  echo "Running ${script}"
  "${odbtcl}" "${script}"
done
