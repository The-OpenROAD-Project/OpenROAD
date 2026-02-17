#!/usr/bin/env bash

## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

set -e

#
# Uses the current directory to figure out what the tool name is. Allows us to
# call the script from src/tool/test and src/tool directories. Wish we could
# just use the path of the regression script, but we can't since $0 points to
# the location in test/shared and not src/tool/test.
#
_get_tool_name()
{
    echo $(readlink $0)
    script_path=$(realpath $PWD)
    tool=$(basename "${script_path%%/src/*}/src/$(echo "$script_path" | awk -F'/src/' '{print $2}' | cut -d'/' -f1)")
}

_get_tool_name

# build directory is two directories up from test/shared where this file exists
build_dir_path=$(dirname $(dirname $(dirname $(realpath "$0"))))/build

cd $build_dir_path

ctest -L " $tool " ${@:1}
    

