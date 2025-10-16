#!/bin/bash

###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2024, The Regents of the University of California
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
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
###############################################################################

# This code does the symlink for module-level READMEs to the working folder
#  for manpage compilations.

SRC_BASE_PATH="../src"
DEST_BASE_PATH="./md/man2"
MAN3_DIR="./md/man3"
mkdir -p $DEST_BASE_PATH
mkdir -p $MAN3_DIR  # needed for CI Tests.

# Loop through all folders inside "../src"
for MODULE_PATH in "$SRC_BASE_PATH"/*; do
    if [ -d "$MODULE_PATH" ]; then
        MODULE=$(basename "$MODULE_PATH")
	    SRC_PATH=$(realpath $SRC_BASE_PATH/$MODULE/README.md)
	    DEST_PATH="$(realpath $DEST_BASE_PATH/$MODULE).md"

        # Check if README.md exists before copying
        if [ -e "$SRC_PATH" ]; then
            ln -s -f "$SRC_PATH" "$DEST_PATH"
            echo "File linked successfully."
        else
            echo "ERROR: README.md not found in $MODULE_PATH"
        fi
    fi
done
