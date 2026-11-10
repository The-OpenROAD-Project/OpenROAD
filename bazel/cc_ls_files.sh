#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# List the C/C++ files that clang-format owns, NUL-separated. Submodules
# (e.g. src/sta) are already left out by git_ls_files.sh; generator templates
# and vendored third-party code are excluded here, as in the clang-format
# GitHub workflows.

set -euo pipefail

GIT_LS_FILES="$1"
GIT="$2"

"${GIT_LS_FILES}" "${GIT}" -z \
    '*.h' '*.hh' '*.hpp' '*.hxx' '*.c' '*.cc' '*.cpp' '*.cxx' \
    ':^src/odb/src/codeGenerator/' \
    ':^third-party/'
