#!/bin/bash

set -euo pipefail

cd "$(dirname $(readlink -f $0))/../"

# default values, can be overwritten by cmdline args
buildDir="build"
numThreads="$(nproc)"
cmakeOptions=("")
cleanBefore=no
keepLog=no
compiler=gcc

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
  -no-warnings
                                                Compiler warnings are
                                                considered errors, i.e.,
                                                use -Werror flag during build.
  -dir=PATH                                     Path to store build files.
                                                  Default: ./build
  -coverage                                     Enable cmake coverage options
  -clean                                        Remove build dir before compile
  -no-gui                                       Disable GUI support
  -threads=NUM_THREADS                          Number of threads to use during
                                                  compile. Default: \`nproc\`
  -keep-log                                     Keep a compile log in build dir
  -help                                         Shows this message
  -gpu                                          Enable GPU to accelerate the process

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
        -no-warnings )
            cmakeOptions+=( -DALLOW_WARNINGS=OFF )
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
        -gpu)
            cmakeOptions+=( -DGPU=ON )
            ;;
        *)
            echo "unknown option: ${1}" >&2
            _help
            ;;
    esac
    shift 1
done

case "${compiler}" in
    "gcc" )
        if [[ -f "/opt/rh/devtoolset-8/enable" ]]; then
            # the scl script has unbound variables
            set +u
            source /opt/rh/devtoolset-8/enable
            set -u
        fi
        export CC="$(command -v gcc)"
        export CXX="$(command -v g++)"
        ;;
    "clang" )
        if [[ -f "/opt/rh/llvm-toolset-7.0/enable" ]]; then
            # the scl script has unbound variables
            set +u
            source /opt/rh/llvm-toolset-7.0/enable
            set -u
        fi
        export CC="$(command -v clang)"
        export CXX="$(command -v clang++)"
        ;;
    *)
        echo "Compiler $compiler is not supported" >&2
        _help 1
esac

if [[ "${cleanBefore}" == "yes" ]]; then
    rm -rf "${buildDir}"
fi

mkdir -p "${buildDir}"
if [[ "${keepLog}" == "yes"  ]]; then
    logName="${buildDir}/openroad-build-$(date +%s).log"
else
    logName=/dev/null
fi

cmake "${cmakeOptions[@]}" -B "${buildDir}" . 2>&1 | tee "${logName}"
time cmake --build "${buildDir}" -j "${numThreads}" 2>&1 | tee -a "${logName}"
