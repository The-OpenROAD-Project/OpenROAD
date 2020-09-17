#!/bin/bash
set -x
set -e

if [[ $# -ne 2 ]]; then
    echo "usage: $0 TARGET_OS TARGET_COMPILER"
    exit 1
fi

TARGET_OS="$1"
TARGET_COMPILER="$2"

DOCKER_TAG="openroad/openroad_${TARGET_OS}_${TARGET_COMPILER}"

docker run --rm "$DOCKER_TAG" bash -c "./test/regression"
