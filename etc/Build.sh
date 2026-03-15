#!/usr/bin/env bash

set -euo pipefail

DIR="$(dirname $(readlink -f $0))"
cd "$DIR/../"

# default values, can be overwritten by cmdline args
buildDir="build"
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
cmakeOptions=()
isNinja=no
cleanBefore=no
depsPrefixesFile=""
compiler=gcc

_help() {
    cat <<EOF
usage: $0 [OPTIONS]

OPTIONS:
  -cmake='-<key>=<value> [-<key>=<value> ...]'  User defined cmake options
                                                  Note: use single quote after
                                                  -cmake= and double quotes if
                                                  <key> has multiple <values>
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
  -no-tests                                     Disable GTest
  -ninja                                        Use Ninja build system
  -cpp20                                        Use C++20 standard
  -build-man                                    Build Man Pages (optional)
  -threads=NUM_THREADS                          Number of threads to use during
                                                  compile. Default: \`nproc\` on linux
                                                  or \`sysctl -n hw.logicalcpu\` on macOS
  -keep-log                                     Keep a compile log in build dir
  -help                                         Shows this message
  -gpu                                          Enable GPU to accelerate the process
  -deps-prefixes-file=FILE                      File with CMake packages roots,
                                                  its content extends -cmake argument.
                                                  By default, "openroad_deps_prefixes.txt"
                                                  file from OpenROAD's "etc" directory
                                                  or from system "/etc".
  -local                                        Install OpenROAD in \${HOME}/.local.
  -prefix=DIR                                   Install OpenROAD in a user-specified directory.

EOF
    exit "${1:-1}"
}

__logging()
{
        local log_file="${buildDir}/openroad_build.log"
        echo "[INFO] Saving logs to ${log_file}"
        echo "[INFO] $__CMD"
        exec > >(tee -i "${log_file}")
        exec 2>&1
}

__CMD="$0 $@"
while [ "$#" -gt 0 ]; do
    case "${1}" in
        -h|-help)
            _help 0
            ;;
        -local)
            if [[ -n "${INSTALL_PREFIX_SET:-}" ]]; then
                echo "[WARNING] Previous -prefix or -local argument will be overwritten." >&2
            fi
            cmakeOptions+=("-DCMAKE_INSTALL_PREFIX=${HOME}/.local")
            INSTALL_PREFIX_SET=1
            ;;
        -prefix=*)
            if [[ -n "${INSTALL_PREFIX_SET:-}" ]]; then
                echo "[WARNING] Previous -prefix or -local argument will be overwritten." >&2
            fi
            cmakeOptions+=("-DCMAKE_INSTALL_PREFIX=${1#*=}")
            INSTALL_PREFIX_SET=1
            ;;
        -no-gui)
            cmakeOptions+=("-DBUILD_GUI=OFF")
            ;;
        -no-tests)
            cmakeOptions+=("-DENABLE_TESTS=OFF")
            ;;
        -ninja)
            cmakeOptions+=("-DCMAKE_C_COMPILER_LAUNCHER=ccache" "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache" "-GNinja")
            isNinja=yes
            ;;
        -cpp20)
            cmakeOptions+=("-DCMAKE_CXX_STANDARD=20")
            ;;
        -build-man)
            cmakeOptions+=("-DBUILD_MAN=ON")
            ;;
        -compiler=*)
            compiler="${1#*=}"
            ;;
        -no-warnings )
            cmakeOptions+=("-DALLOW_WARNINGS=OFF")
            ;;
        -coverage )
            cmakeOptions+=("-DCMAKE_BUILD_TYPE=Debug" "-DCMAKE_CXX_FLAGS=-fprofile-arcs -ftest-coverage" "-DCMAKE_EXE_LINKER_FLAGS=-lgcov")
            ;;
        -cmake=*)
            eval "temp_arr=(${1#*=})"
            cmakeOptions+=("${temp_arr[@]}")
            ;;
        -clean )
            cleanBefore=yes
            ;;
        -dir=* )
            buildDir="${1#*=}"
            ;;
        -threads=* )
            temp="${1#*=}"
            if [[ "${temp}" != "NotSet" ]]; then
                numThreads=${temp}
            fi
            ;;
        -deps-prefixes-file=*)
            file="${1#-deps-prefixes-file=}"
            if [[ ! -f "$file" ]]; then 
                echo "${file} does not exist" >&2
                _help
            fi
            depsPrefixesFile="$file"
            ;;
        -compiler | -cmake | -dir | -threads | -install | -deps-prefixes-file )
            echo "${1} requires an argument" >&2
            _help
            ;;
        -gpu)
            cmakeOptions+=("-DGPU=ON")
            ;;
        *)
            echo "unknown option: ${1}" >&2
            _help
            ;;
    esac
    shift 1
done

if [[ -z "$depsPrefixesFile" ]]; then
    if [[ -f "$DIR/openroad_deps_prefixes.txt" ]]; then
        depsPrefixesFile="$DIR/openroad_deps_prefixes.txt"
    elif [[ -f "/etc/openroad_deps_prefixes.txt" ]]; then
        depsPrefixesFile="/etc/openroad_deps_prefixes.txt"
    fi
fi
if [[ -f "$depsPrefixesFile" ]]; then
    while read -r dep; do
        if [[ -n "$dep" && "$dep" != \#* ]]; then
            cmakeOptions+=("$dep")
        fi
    done < <(xargs -n1 < "$depsPrefixesFile")
    echo "[INFO] Using additional CMake parameters from $depsPrefixesFile"
else
    echo "[INFO] Auto-generated prefix file does not exist - CMake will choose the dependencies automatically"
fi

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
    "clang-16" )
        export CC="$(command -v clang-16)"
        export CXX="$(command -v clang++-16)"
        ;;
    *)
        export CC=""
        export CXX=""
esac

if [[ -z "${CC}" || -z "${CXX}" ]]; then
        echo "Compiler $compiler not installed or it is not supported." >&2
        _help 1
fi

if [[ "${cleanBefore}" == "yes" ]]; then
    rm -rf "${buildDir}"
fi

mkdir -p "${buildDir}"
__logging

if [[ "$OSTYPE" == "darwin"* ]]; then
    export PATH="$(brew --prefix bison)/bin:$(brew --prefix flex)/bin:$PATH"
    export CMAKE_PREFIX_PATH=$(brew --prefix or-tools)
fi

echo "[INFO] Using ${numThreads} threads."
if [[ "$isNinja" == "yes" ]]; then
    cmake "${cmakeOptions[@]}" -B "${buildDir}" .
    cd "${buildDir}"
    CLICOLOR_FORCE=1 ninja build_and_test
    exit 0
fi
cmake "${cmakeOptions[@]}" -B "${buildDir}" .
time cmake --build "${buildDir}" -j "${numThreads}"
