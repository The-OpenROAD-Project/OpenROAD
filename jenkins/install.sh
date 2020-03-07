#!/bin/bash
set -x
set -e
mkdir -p /OpenROAD/build
cd /OpenROAD
cmake -B build
cmake --build build -j 4

# Capture the commit we are testing for use in flow testing
commit=`git rev-parse --verify HEAD`

# Build a subdir 'flow' to run OpenROAD-flow tests in
if [[ ! -d flow ]]; then
    git clone -b openroad https://github.com/The-OpenROAD-Project/OpenROAD-flow.git flow
fi

cd flow

# Get the head of the openroad branch of OpenROAD-flow
git pull --ff-only
git submodule update --init --recursive

# Swap to current commit of OpenROAD
cd tools/OpenROAD
if ! git ls-remote local > /dev/null; then
    git remote add local ../../..
fi
git fetch
git fetch local
git checkout ${commit}
git submodule update --init --recursive
