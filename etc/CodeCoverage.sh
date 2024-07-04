#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname $(readlink -f $0))/../"

_help() {
    cat <<EOF
usage: $0 [dynamic]
       $0 static

EOF
    exit "${1:-1}"
}

_lcov() {
    ctest --test-dir build -j $(nproc)

    # sta has a private test suite; mpl is obsoleted by mpl2;
    # drt's gr is not in use
    mkdir -p coverage-output
    lcov \
        --capture \
        --directory ./build \
        --exclude "/usr/include/*" \
        --exclude "/opt/*" \
        --exclude "/usr/lib/*" \
        --exclude "/usr/local/*" \
        --exclude "*/.local/*" \
        --exclude "*build*" \
        --exclude "*/third-party/*" \
        --exclude "*/sta/*" \
        --exclude "*/test/*" \
        --exclude "*/mpl/*" \
        --exclude "*/drt/src/gr/*" \
        --exclude "*/drt/src/db/grObj/*" \
        --output-file ./coverage-output/main_coverage.info

    genhtml ./coverage-output/main_coverage.info \
        --output-directory ./coverage-output \
        --ignore-errors source

}

_coverity() {
    cmake -B build .
    # compile abc before calling cov-build to exclude from analysis.
    # Coverity fails to process abc code due to -fpermissive flag.
    cmake --build build -j $(nproc) --target abc
    cov-build --dir cov-int cmake --build build -j $(nproc)
    log_file=cov-int/build-log.txt
    regex='Emitted.*compilation units.*\(\d+%\)'
    # get compilation coverage percentage
    percent=$(grep -Poi "${regex}" ${log_file} | grep -Po '\d+' | tail -n 1)
    if [[ ${percent} -lt 85  ]]; then
        echo "Coverity requires more than 85% of compilation coverage. Only got ${percentage}%."
        exit 1
    fi
    tar czvf openroad.tgz cov-int
    commitSha="$(git rev-parse HEAD)"
    curl --form token=$COVERITY_TOKEN \
         --form email=openroad@eng.ucsd.edu \
         --form file=@openroad.tgz \
         --form version="$commitSha" \
         "https://scan.coverity.com/builds?project=The-OpenROAD-Project%2FOpenROAD"
}

target="${1:-dynamic}"
case "${target}" in
    dynamic )
        _lcov
        ;;
    static )
        _coverity
        ;;
    *)
        echo "invalid argument: ${1}" >&2
        _help
        ;;
esac
