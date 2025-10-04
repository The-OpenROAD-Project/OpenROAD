#!/usr/bin/env bash

cd $(dirname $(realpath $0))/../

if [[ "$@" == "-dev" ]]; then
    file_list=(
        "./docker/Dockerfile.binary"
        "./docker/Dockerfile.builder"
        "./docker/Dockerfile.dev"
        "./etc/DependencyInstaller.sh"
    )
    cat "${file_list[@]}" | sha256sum | awk '{print substr($1, 1, 6)}'
elif [[ "$@" == "-bazel" ]]; then
    file_list=(
        "./docker/Dockerfile.bazel"
    )
    cat "${file_list[@]}" | sha256sum | awk '{print substr($1, 1, 6)}'
elif [[ "$@" == "-master" ]]; then
    git fetch --tags >&2
    git describe
else
    echo "Usage:"
    echo " To generate a tag for images that only have dev dependencies:"
    echo "       $0 -dev"
    echo " To generate a tag for images that use bazel (mainly for CI):"
    echo "       $0 -bazel"
    echo " To generate a tag for full releases based on master branch"
    echo "       $0 -master"
    exit 1
fi
