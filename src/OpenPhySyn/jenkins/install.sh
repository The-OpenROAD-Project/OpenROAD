#/bin/bash
set -x
set -e
mkdir -p /OpenPhySyn/build
cd /OpenPhySyn/build
cmake .. -DCMAKE_BUILD_TYPE=release
make -j $(nproc)
