#!/usr/bin/env bash

if [[ $# -ne 2  ]]; then
    echo "Error: This script requires exactly one argument."
    echo "usage: $0 <VERSION> <OS_NAME>"
    exit 1
fi

for file in ../*${1}*; do
    base_name=$(basename "$file")
    name="${base_name%.*}"
    ext="${base_name##*.}"
    mv "$file" "${name}-${2}.${ext}"
done
