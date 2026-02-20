#!/bin/bash

## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

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
