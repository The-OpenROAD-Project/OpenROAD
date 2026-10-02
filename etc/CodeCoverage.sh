#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname $(readlink -f $0))/../"

_help() {
    cat <<EOF
usage: $0 [dynamic|test]
       $0 static <TOKEN>
       $0 static-bazel <TOKEN>
       $0 upload <TOKEN> [VERSION]

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

_coverity_cmake() {
    cmakeOptions=""
    if [[ -f "/etc/openroad_deps_prefixes.txt" ]]; then
        cmakeOptions="$(cat "/etc/openroad_deps_prefixes.txt")"
    fi
    cmake ${cmakeOptions} -B build .
    # compile abc before calling cov-build to exclude from analysis.
    # Coverity fails to process abc code due to -fpermissive flag.
    cmake --build build -j $(nproc) --target abc
    cov-build --dir cov-int cmake --build build -j $(nproc)
}

_coverity_bazel() {
    # Coverity captures compiler invocations, so cached or sandboxed Bazel
    # actions can make the capture incomplete. Clear the local action cache,
    # disable the persistent action caches, and execute spawns locally.
    bazelisk clean
    cov-build --dir cov-int --bazel bazelisk build \
        --spawn_strategy=local \
        --remote_cache= \
        --disk_cache= \
        --noremote_accept_cached \
        --noremote_upload_local_results \
        --jobs=$(nproc) \
        --//:platform=cli \
        -- //:openroad
}

_coverity_capture() {
    "$1"
    log_file=cov-int/build-log.txt
    # get compilation coverage percentage
    percent=$(sed -nE \
        's/.*Emitted.*compilation units.*\(([0-9]+)%\).*/\1/p' \
        "${log_file}" | tail -n 1)
    # An empty match means the capture summary is missing; treat it as 0%.
    percent="${percent:-0}"
    if [[ ${percent} -lt 85 ]]; then
        echo "Coverity requires more than 85% of compilation coverage. Only got ${percent}%."
        exit 1
    fi

    tar czvf openroad.tgz cov-int
    git rev-parse HEAD > openroad.version
}

_coverity_init_error() {
    local response
    response=$(tr '\n' ' ' < "$1")
    response="${response% }"
    if [[ -z ${response} ]]; then
        response="empty response"
    fi
    echo "Coverity build initialization failed: ${response}" >&2
}

_coverity_upload() {
    local version="${1:-}"
    if [[ ! -f openroad.tgz ]]; then
        echo "Coverity upload failed: openroad.tgz does not exist." >&2
        return 1
    fi
    if [[ -z ${version} && -f openroad.version ]]; then
        version=$(< openroad.version)
    fi
    if [[ -z ${version} ]]; then
        echo "Coverity upload failed: no source version is available. Pass VERSION for a legacy archive." >&2
        return 1
    fi

    if [ -n "${SKIP_COVERITY_UPLOAD+x}" ]; then
        echo "SKIP_COVERITY_UPLOAD is set. Skipping Coverity upload."
        return 0
    fi

    # Step 1: Initialize a build. Fetch a cloud upload url.
    # Omit --fail so an HTTP error body reaches the jq check in Step 2 and is
    # reported. --fail-with-body needs curl 7.76, which Ubuntu 20.04 lacks.
    local response_file
    response_file=$(mktemp)
    if ! curl --silent --show-error -X POST \
        -d "version=${version}" \
        -d "description=build=${version}" \
        -d email=openroad@ucsd.edu \
        -d "token=${token}" \
        -d file_name=openroad.tgz \
        https://scan.coverity.com/projects/21946/builds/init \
        > "${response_file}"; then
        _coverity_init_error "${response_file}"
        rm -f "${response_file}"
        return 1
    fi

    # Step 2: Store response data to use in later stages.
    local response_fields
    if ! response_fields=$(jq -er '
        select(
            (.url | type) == "string"
            and (.url | length) > 0
            and ((.build_id | type) == "string" or (.build_id | type) == "number")
        )
        | [.url, (.build_id | tostring)]
        | @tsv
    ' "${response_file}" 2>/dev/null); then
        _coverity_init_error "${response_file}"
        rm -f "${response_file}"
        return 1
    fi
    local upload_url
    local build_id
    IFS=$'\t' read -r upload_url build_id <<< "${response_fields}"
    rm -f "${response_file}"

    # Step 3: Upload the tarball to the Cloud.
    if ! curl --fail --silent --show-error -X PUT \
        --header 'Content-Type: application/json' \
        --upload-file openroad.tgz \
        "${upload_url}"; then
        echo "Coverity archive upload failed." >&2
        return 1
    fi

    # Step 4: Trigger the build on Scan.
    if ! curl --fail --silent --show-error -X PUT \
        -d "token=${token}" \
        "https://scan.coverity.com/projects/21946/builds/${build_id}/enqueue"; then
        echo "Coverity enqueue failed." >&2
        return 1
    fi

}

token=""

target="${1:-dynamic}"
case "${target}" in
    dynamic )
        _lcov
        ;;
    test )
        bazelisk test //etc:code_coverage_test
        ;;
    static | static-bazel )
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
        if [[ ${target} == "static-bazel" ]]; then
            _coverity_capture _coverity_bazel
        else
            _coverity_capture _coverity_cmake
        fi
        _coverity_upload
        ;;
    upload )
        if [[ $# -lt 2 || $# -gt 3 ]]; then
            echo "'${0} upload' requires a token and accepts one optional version." >&2
            _help
        fi
        token="${2}"
        _coverity_upload "${3:-}"
        ;;
    *)
        echo "invalid argument: ${1}" >&2
        _help
        ;;
esac
