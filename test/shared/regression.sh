#!/usr/bin/env bash

############################################################################
##
## Copyright (c) 2024, The Regents of the University of California
## All rights reserved.
##
## BSD 3-Clause License
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
############################################################################

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

ctest -L $tool ${@:1}
    

