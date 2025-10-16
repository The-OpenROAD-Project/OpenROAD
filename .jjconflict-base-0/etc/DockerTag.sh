#!/usr/bin/env bash

cd $(dirname $(realpath $0))/../

if [[ "$@" == "-dev" ]]; then
    file_list=(
        "./docker/Dockerfile.binary"
        "./docker/Dockerfile.builder"
        "./docker/Dockerfile.dev"
        "./etc/Build.sh"
        "./etc/DependencyInstaller.sh"
        "./etc/DockerHelper.sh"
        "./etc/DockerTag.sh"
    )
    cat "${file_list[@]}" | sha256sum | awk '{print substr($1, 1, 6)}'
elif [[ "$@" == "-master" ]]; then
    git describe
else
    echo "Usage: $0 {-dev|-master}"
    exit 1
fi
