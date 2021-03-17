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
DOCKER_FILE="./jenkins/docker/Dockerfile.${TARGET_OS}.${TARGET_COMPILER}"

docker build --tag "$DOCKER_TAG" --target=builder -f  "$DOCKER_FILE" .
