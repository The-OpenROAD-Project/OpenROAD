#!/bin/bash
set -x
set -e
if [[ $# -eq 2 ]]; then
    TARGET_OS="$1"
    TARGET_COMPILER="$2"
    docker build -t "openroad/openroad_${TARGET_OS}_${TARGET_COMPILER}" --target builder -f "./jenkins/docker/Dockerfile.${TARGET_OS}.${TARGET_COMPILER}" .
else
    docker build -t openroad/openroad --target base-dependencies .
    docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "./OpenROAD/jenkins/install_docker.sh"
fi
