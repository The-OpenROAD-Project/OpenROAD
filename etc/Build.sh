#!/bin/bash

set -eo pipefail

cd "$(dirname $(readlink -f $0))/../"

buildDir="build"
numThreads="$(nproc)"
cmakeOptions=()

_help() {
    cat <<EOF
usage: $0 [OPTIONS]

OPTIONS:
  -cmake='-<key>=<value> [-<key>=<value> ...]'  User defined cmake options
                                                  Note: use single quote after
                                                  -cmake= and double quotes if
                                                  <key> has muliple <values>
                                                  e.g.: -cmake='-DFLAGS="-a -b"'
  -compiler=COMPILER_NAME                       Compiler name: gcc or clang
                                                  Default: gcc
  -dir=PATH                                     Path to store build files.
                                                  Default: ./build
  -coverage                                     Enable cmake coverage options
  -clean                                        Remove build dir before compile
  -no-gui                                       Disable GUI support
  -threads=NUM_THREADS                          Number of threads to use during
                                                  compile. Default: \`nproc\`
  -keep-log                                     Keep a compile log in build dir
  -help                                         Shows this message

EOF
    exit "${1:-1}"
}

while [ "$#" -gt 0 ]; do
    case "${1}" in
        -h|-help)
            _help 0
            ;;
        -no-gui)
            cmakeOptions+=( -DBUILD_GUI=OFF )
            ;;
        -compiler=*)
            compiler="${1#*=}"
            ;;
        -coverage )
            cmakeOptions+=( -DCMAKE_BUILD_TYPE=Debug )
            cmakeOptions+=( -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage" )
            cmakeOptions+=( -DCMAKE_EXE_LINKER_FLAGS=-lgcov )
            ;;
        -cmake=*)
            cmakeOptions+=( "${1#*=}" )
            ;;
        -clean )
            cleanBefore=yes
            ;;
        -dir=* )
            buildDir="${1#*=}"
            ;;
        -keep-log )
            keepLog=yes
            ;;
        -threads=* )
            numThreads="${1#*=}"
            ;;
        -compiler | -cmake | -dir | -threads )
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

case "${compiler:-gcc}" in
    "gcc" )
        if [[ -f "/opt/rh/devtoolset-8/enable" ]]; then
            source /opt/rh/devtoolset-8/enable
        fi
        export CC="$(command -v gcc)"
        export CXX="$(command -v g++)"
        ;;
    "clang" )
        if [[ -f "/opt/rh/llvm-toolset-7.0/enable" ]]; then
            source /opt/rh/llvm-toolset-7.0/enable
        fi
        export CC="$(command -v clang)"
        export CXX="$(command -v clang++)"
        ;;
    *)
        echo "Compiler $compiler is not supported" >&2
        _help 1
esac

if [[ "${cleanBefore:-no}" == "yes" ]]; then
    rm -rf "${buildDir}"
fi

mkdir -p "${buildDir}"
if [[ "${keepLog:-no}" == "yes"  ]]; then
    logName="${buildDir}/openroad-build-$(date +%s).log"
else
    logName=/dev/null
fi

cmake "${cmakeOptions[@]}" -B "${buildDir}" . | tee "${logName}"
time cmake --build "${buildDir}" -j "${numThreads}" | tee -a "${logName}"
