#!/bin/bash
set -x
set -e
./test/regression
lcov --capture --directory ./build --exclude '/usr/include/*' --exclude '/opt/*' --exclude '/usr/lib/*' --exclude '/usr/local/*' --exclude '*build*' --output-file ./main_coverage.info
genhtml ./main_coverage.info --output-directory ./out --ignore-errors source
