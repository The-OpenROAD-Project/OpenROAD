#!/usr/bin/env bash

set -e

tool=$(basename $(dirname $PWD))

cd ../../../build

ctest -L $tool ${@:1}
