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
  -os=OS_NAME                   Choose beween ubuntu22.04 (default), ubuntu20.04, rhel, opensuse, debian10 and debian11.
  -target=TARGET                Choose target fo the Docker image:
                                  'dev': os + packages to compile app
                                  'builder': os + packages to compile app +
                                             copy source code and build app
                                  'binary': os + packages to run a compiled
                                            app + binary set as entrypoint
  -compiler=COMPILER_NAME       Choose between gcc (default) and clang. Valid
                                  only if the target is 'builder'.
  -threads                      Max number of threads to use if compiling.
                                  Default = \$(nproc)
  -ci                           Install CI tools in image
  -h -help                      Show this message and exits
  -local                        Installs with prefix /home/openroad-deps
  -username                     Docker Username
  -password                     Docker Password

EOF
    exit "${1:-1}"
}

_setup() {
    case "${compiler}" in
        "gcc" | "clang" )
            ;;
        * )
            echo "Compiler ${compiler} not supported" >&2
            _help
            ;;
    esac
    case "${os}" in
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
    if [[ "${tag}" != "" ]]; then
        imageTag="${tag}"
    else
        imageTag=$(./etc/DockerTag.sh -dev)
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
            if [[ "$CI" == "yes" ]]; then
                buildArgs="${buildArgs} -ci"
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
    docker run --rm "${imagePath}" "./docker/test_wrapper.sh" "${compiler}" "ctest --test-dir build -j ${numThreads}"
}

_create() {
    echo "Create docker image ${imagePath} using ${file}"
    eval docker build \
        --file "${file}" \
        --tag "${imagePath}" \
        ${buildArgs} \
        "${context}"
}

_push() {
    if [[ -z ${username+x} ]]; then
        echo "Missing required -username=<USER> argument"
        _help
    fi
    if [[ -z ${password+x} ]]; then
        echo "Missing required -password=<PASS> argument"
        _help
    fi
    if [[ "${target}" != "dev" ]] && [[ "${target}" != "master" ]]; then
        echo "Target ${target} is not valid candidate for push to Docker Hub." >&2
        _help
    fi

    docker login --username "${username}" --password "${password}"

    if [[ "${tag}" == "" ]]; then
        tag=$(./etc/DockerTag.sh -dev)
    fi

    mkdir -p build
    ./etc/DockerHelper.sh create -os=${os} -target=dev -tag=${tag} -ci \
        2>&1 | tee build/create-${os}-dev-${tag}.log

    docker push "${imageName}:${tag}"
}

#
# MAIN
#

# script has at least 1 argument, the rule
if [[ $# -lt 1 ]]; then
    echo "Too few arguments" >&2
    _help
fi

if [[ "$1" == "-h" || "$1" == "-help" ]]; then
    _help 0
fi

_rule="_${1}"
shift 1

# check if the rule is exists
if [[ -z $(command -v "${_rule}") ]]; then
    echo "Command ${_rule/_/} not found" >&2
    _help
fi

# default values, can be overwritten by cmdline args
os="ubuntu22.04"
target="dev"
compiler="gcc"
numThreads="-1"
tag=""
isLocal="no"
equivalenceDeps="yes"
CI="no"
LOCAL_PATH="/home/openroad-deps"

while [ "$#" -gt 0 ]; do
    case "${1}" in
        -h|-help)
            _help 0
            ;;
        -local )
            isLocal=yes
            ;;
        -ci )
            CI="yes"
            ;;
        -no_eqy )
            equivalenceDeps=no
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
        -compiler=*)
            compiler="${1#*=}"
            ;;
        -username=* )
            username="${1#*=}"
            ;;
        -password=* )
            password="${1#*=}"
            ;;
        -tag=* )
            tag="${1#*=}"
            ;;
        -os | -target | -compiler | -threads | -username | -password | -tag )
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

if [[ "${numThreads}" == "-1" ]]; then
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        numThreads=$(nproc --all)
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        numThreads=$(sysctl -n hw.ncpu)
    else
        numThreads=2
        cat << EOF
[WARNING] Unsupported OSTYPE: cannot determine number of host CPUs"
  Defaulting to 2 threads. Use --threads N to use N threads"
EOF
    fi
fi

_setup

"${_rule}"
