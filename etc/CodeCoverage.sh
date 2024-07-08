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

    # Step 1: Initialize a build. Fetch a cloud upload url.
    curl -X POST \
        -d version="version=${commitSha}" \
        -d description="OpenROAD Build ${commitSha}" \
        -d email=openroad@ucsd.edu \
        -d token=${COVERITY_TOKEN} \
        -d file_name=openroad.tgz \
        https://scan.coverity.com/projects/21946/builds/init \
        | tee response

    # Step 2: Store response data to use in later stages.
    # Requires the JSON parsing tool jq.
    # If opting for other bash tools, be careful about url encodings.
    upload_url=$(jq -r '.url' response)
    build_id=$(jq -r '.build_id' response)

    # Step 3: Upload the tarball to the Cloud.
    curl -X PUT \
        --header 'Content-Type: application/json' \
        --upload-file openroad.tgz \
        "${upload_url}"

    # Step 4: Trigger the build on Scan.
    curl -X PUT \
        -d "token=${COVERITY_TOKEN}" \
        https://scan.coverity.com/projects/21946/builds/${build_id}/enqueue

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
