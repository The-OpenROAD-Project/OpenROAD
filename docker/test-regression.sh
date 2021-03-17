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

if [[ $TARGET_OS == centos7 ]] && [[ $TARGET_COMPILER == clang7 ]]; then
  docker run --rm "$DOCKER_TAG" bash -c "scl enable llvm-toolset-7.0 ./test/regression"
else
  docker run --rm "$DOCKER_TAG" bash -c "./test/regression"
fi
