#!/bin/bash
set -x
set -e
TARGET_OS="$1"
TARGET_COMPILER="$2"
docker build -t "openroad/openroad_${TARGET_OS}_${TARGET_COMPILER}" --target builder -f "./jenkins/docker/Dockerfile.${TARGET_OS}.${TARGET_COMPILER}" .
