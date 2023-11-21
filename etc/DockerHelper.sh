#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname $(readlink -f $0))/../"

baseDir="$(pwd)"
# docker hub organization/user from where to pull/push images
org=openroad

_help() {
    cat <<EOF
usage: $0 [CMD] [OPTIONS]

  CMD:
  create                        Create a docker image
  test                          Test the docker image
  push                          Push the docker image to Docker Hub

  OPTIONS:
  -compiler=COMPILER_NAME       Choose between gcc (default) and clang. Valid
                                  only if the target is 'builder'.
  -os=OS_NAME                   Choose beween centos7 (default), ubuntu20.04, ubuntu22.04, rhel, opensuse, debian10 and debian11.
  -target=TARGET                Choose target fo the Docker image:
                                  'dev': os + packages to compile app
                                  'builder': os + packages to compile app +
                                             copy source code and build app
                                  'binary': os + packages to run a compiled
                                            app + binary set as entrypoint
  -threads                      Max number of threads to use if compiling.
                                  Default = \$(nproc)
  -sha                          Use git commit sha as the tag image. Default is
                                  'latest'.
  -h -help                      Show this message and exits
  -local                        Installs with prefix /home/openroad-deps

EOF
    exit "${1:-1}"
}

_setup() {
    commitSha="$(git rev-parse HEAD)"
    case "${compiler}" in
        "gcc" | "clang" )
            ;;
        * )
            echo "Compiler ${compiler} not supported" >&2
            _help
            ;;
    esac
    case "${os}" in
        "centos7")
            osBaseImage="centos:centos7"
            ;;
        "ubuntu20.04")
            osBaseImage="ubuntu:20.04"
            ;;
        "ubuntu22.04")
            osBaseImage="ubuntu:22.04"
            ;;
        "opensuse")
            osBaseImage="opensuse/leap"
            ;;
        "debian10")
            osBaseImage="debian:buster"
            ;;
        "debian11")
            osBaseImage="debian:bullseye"
            ;;
        "rhel")
            osBaseImage="redhat/ubi8"
            ;;
        *)
            echo "Target OS ${os} not supported" >&2
            _help
            ;;
    esac
    imageName="${IMAGE_NAME_OVERRIDE:-"${org}/${os}-${target}"}"
    if [[ "${useCommitSha}" == "yes" ]]; then
        imageTag="${commitSha}"
    else
        imageTag="latest"
    fi
    case "${target}" in
        "builder" )
            fromImage="${FROM_IMAGE_OVERRIDE:-"${org}/${os}-dev"}:${imageTag}"
            context="."
            buildArgs="--build-arg compiler=${compiler}"
            buildArgs="${buildArgs} --build-arg numThreads=${numThreads}"
            if [[ "${isLocal}" == "yes" ]]; then
                buildArgs="${buildArgs} --build-arg LOCAL_PATH=${LOCAL_PATH}/bin"
            fi
            imageName="${IMAGE_NAME_OVERRIDE:-"${imageName}-${compiler}"}"
            ;;
        "dev" )
            fromImage="${FROM_IMAGE_OVERRIDE:-$osBaseImage}"
            context="etc"
            buildArgs=""
            if [[ "${isLocal}" == "yes" ]]; then
                buildArgs="-prefix=${LOCAL_PATH}"
            fi
            if [[ "${equivalenceDeps}" == "yes" ]]; then
                buildArgs="${buildArgs} -eqy"
            fi
            if [[ "${buildArgs}" != "" ]]; then
                buildArgs="--build-arg INSTALLER_ARGS='${buildArgs}'"
            fi
            ;;
        "binary" )
            fromImage="${FROM_IMAGE_OVERRIDE:-${org}/${os}-dev}:${imageTag}"
            context="etc"
            copyImage="${COPY_IMAGE_OVERRIDE:-"${org}/${os}-builder-${compiler}"}:${imageTag}"
            buildArgs="--build-arg copyImage=${copyImage}"
            ;;
        *)
            echo "Target ${target} not found" >&2
            _help
            ;;
    esac
    imagePath="${imageName}:${imageTag}"
    buildArgs="--build-arg fromImage=${fromImage} ${buildArgs}"
    file="docker/Dockerfile.${target}"
}

_test() {
    echo "Run regression test on ${imagePath}"
    case "${target}" in
        "builder" )
            ;;
        *)
            echo "Target ${target} is not valid candidate to run regression" >&2
            _help
            ;;
    esac
    if [[ "$(docker images -q ${imagePath} 2> /dev/null)" == "" ]]; then
        echo "Could not find ${imagePath}, will attempt to create it" >&2
        _create
    fi
    docker run --rm "${imagePath}" "./docker/test_wrapper.sh" "${compiler}" "./test/regression"
}

_create() {
    echo "Create docker image ${imagePath} using ${file}"
    eval docker build --file "${file}" --tag "${imagePath}" ${buildArgs} "${context}"
}

_push() {
    case "${target}" in
        "dev" )
            read -p "Will push docker image ${imagePath} to DockerHub [y/N]" -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$  ]]; then
                mkdir -p build

                OS_LIST="centos7 ubuntu20.04 ubuntu22.04"
                # create image with sha and latest tag for all os
                for os in ${OS_LIST}; do
                    ./etc/DockerHelper.sh create -target=dev \
                        2>&1 | tee build/create-${os}-latest.log &
                done
                wait

                for os in ${OS_LIST}; do
                    ./etc/DockerHelper.sh create -target=dev -sha \
                        2>&1 | tee build/create-${os}-${commitSha}.log &
                done
                wait

                # test image with sha and latest tag for all os and compiler
                for os in ${OS_LIST}; do
                    ./etc/DockerHelper.sh test -target=builder -sha \
                        2>&1 | tee build/test-${os}-gcc-latest.log &
                done
                wait

                for os in ${OS_LIST}; do
                    ./etc/DockerHelper.sh test -target=builder -sha -compiler=clang \
                        2>&1 | tee build/test-${os}-clang-latest.log &
                done
                wait

                for os in ${OS_LIST}; do
                    echo [DRY-RUN] docker push openroad/${os}-dev:latest
                    echo [DRY-RUN] docker push openroad/${os}-dev:${commitSha}
                done

            else
                echo "Will not push."
            fi
            ;;
        *)
            echo "Target ${target} is not valid candidate for push to DockerHub." >&2
            _help
            ;;
    esac
}

#
# MAIN
#

# script has at least 1 argument, the rule
if [[ $# -lt 1 ]]; then
    echo "Too few arguments" >&2
    _help
fi

_rule="_${1}"
shift 1

# check if the rule is exists
if [[ -z $(command -v "${_rule}") ]]; then
    echo "Command ${_rule/_/} not found" >&2
    _help
fi

# default values, can be overwritten by cmdline args
os="centos7"
target="dev"
compiler="gcc"
useCommitSha="no"
isLocal="no"
equivalenceDeps="yes"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
  numThreads=$(nproc --all)
elif [[ "$OSTYPE" == "darwin"* ]]; then
  numThreads=$(sysctl -n hw.ncpu)
else
  cat << EOF
WARNING: Unsupported OSTYPE: cannot determine number of host CPUs"
  Defaulting to 2 threads. Use --threads N to use N threads"
EOF
  numThreads=2
fi
LOCAL_PATH="/home/openroad-deps"

while [ "$#" -gt 0 ]; do
    case "${1}" in
        -h|-help)
            _help 0
            ;;
        -compiler=*)
            compiler="${1#*=}"
            ;;
        -os=* )
            os="${1#*=}"
            ;;
        -target=* )
            target="${1#*=}"
            ;;
        -threads=* )
            numThreads="${1#*=}"
            ;;
        -sha )
            useCommitSha=yes
            ;;
        -local )
            isLocal=yes
            ;;
        -no_eqy )
            equivalenceDeps=no
            ;;
        -compiler | -os | -target )
            echo "${1} requires an argument" >&2
            _help
            ;;
        *)
            echo "unknown option: ${1}" >&2
            _help
            ;;
    esac
    shift 1
done

_setup

"${_rule}"
