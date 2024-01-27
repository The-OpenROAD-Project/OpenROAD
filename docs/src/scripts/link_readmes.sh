#!/bin/bash

SRC_BASE_PATH="../src"
DEST_BASE_PATH="./md/man2"
mkdir -p $DEST_BASE_PATH

# Loop through all folders inside "../src"
for MODULE_PATH in "$SRC_BASE_PATH"/*; do
    if [ -d "$MODULE_PATH" ]; then
        MODULE=$(basename "$MODULE_PATH")
	    SRC_PATH=$(realpath $SRC_BASE_PATH/$MODULE/README.md)
	    DEST_PATH="$(realpath $DEST_BASE_PATH/$MODULE).md"

        # Check if README.md exists before copying
        if [ -e "$SRC_PATH" ]; then
            ln -s -f "$SRC_PATH" "$DEST_PATH"
            echo "File linked from $SRC_PATH to $DEST_PATH"
        else
            echo "ERROR: README.md not found in $MODULE_PATH"
        fi
    fi
done
