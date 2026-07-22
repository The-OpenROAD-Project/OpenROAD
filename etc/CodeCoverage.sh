#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname $(readlink -f $0))/../"

_help() {
    cat <<EOF
usage: $0 [dynamic]
       $0 static <TOKEN>

EOF
    exit "${1:-1}"
}

_lcov() {
    ctest --test-dir build -j $(nproc)

    # sta has a private test suite
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
        --exclude "*/drt/src/gr/*" \
        --exclude "*/drt/src/db/grObj/*" \
        --output-file ./coverage-output/main_coverage.info

    genhtml ./coverage-output/main_coverage.info \
        --output-directory ./coverage-output \
        --ignore-errors source

}

_coverity() {
    cmakeOptions=""
    if [[ -f "/etc/openroad_deps_prefixes.txt" ]]; then
        cmakeOptions="$(cat "/etc/openroad_deps_prefixes.txt")"
    fi
    cmake ${cmakeOptions} -B build .
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

    if [ -n "${SKIP_COVERITY_UPLOAD+x}" ]; then
        echo "SKIP_COVERITY_UPLOAD is set. Skipping Coverity upload."
        exit 0
    fi

    # Step 1: Initialize a build. Fetch a cloud upload url.
    curl -X POST \
        -d version="version=${commitSha}" \
        -d description="build=${commitSha}" \
        -d email=openroad@ucsd.edu \
        -d token=${token} \
        -d file_name=openroad.tgz \
        https://scan.coverity.com/projects/21946/builds/init \
        | tee response

    cat response

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
        -d "token=${token}" \
        https://scan.coverity.com/projects/21946/builds/${build_id}/enqueue

}

token=""

target="${1:-dynamic}"
case "${target}" in
    dynamic )
        _lcov
        ;;
    static )
        if [[ $# -ne 2 ]]; then
            if [[ $# -lt 2 ]]; then
                echo -n "Too few arguments. "
            fi
            if [[ $# -gt 2 ]]; then
                echo -n "Too many arguments. "
            fi
            echo "'${0} ${1}' requires a token as the second argument."
            _help
        fi
        token="${2}"
        _coverity
        ;;
    *)
        echo "invalid argument: ${1}" >&2
        _help
        ;;
esac
