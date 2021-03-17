#!/bin/bash
set -x
set -e

if [[ "$#" -gt 0 ]]; then
    case "$@" in
        "coverge" )
            IS_COVERAGE=1
            ;;
        *)
            echo "Option not recognized"
            echo "usage: $0"
            echo "       $0 [coverage]"
            return 1
            ;;
    esac
fi

./test/regression

if [[ -z "$IS_COVERAGE" ]]; then
    mkdir -p coverage-output
    lcov --capture --directory ./build --exclude '/usr/include/*' --exclude '/opt/*' --exclude '/usr/lib/*' --exclude '/usr/local/*' --exclude '*build*' --output-file ./coverage-output/main_coverage.info
    genhtml ./coverage-output/main_coverage.info --output-directory ./coverage-output --ignore-errors source
fi
