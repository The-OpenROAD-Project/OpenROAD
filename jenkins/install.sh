#!/bin/bash
set -x
set -e
mkdir -p /OpenROAD/build
cd /OpenROAD/build
cmake ..
make -j 4

echo `whoami`

# Capture the commit we are testing for use in flow testing
commit=`git rev-parse --verify HEAD`

# setup OpenROAD-flow
cd /OpenROAD-flow
if [[ ! -d .git ]]; then
    git clone -b openroad https://github.com/The-OpenROAD-Project/OpenROAD-flow.git /
else
    git fetch
fi

git checkout openroad
git submodule update --init --recursive
(cd tools/OpenROAD;
 git checkout ${commit};
 git submodule update --init --recursive)
