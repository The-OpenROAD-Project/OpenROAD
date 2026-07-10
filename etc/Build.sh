#!/usr/bin/env bash

set -euo pipefail

DIR="$(dirname $(readlink -f $0))"
cd "$DIR/../"

# default values, can be overwritten by cmdline args
buildDir="build"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
  numThreads=$(nproc --all)
elif [[ "$OSTYPE" == "darwin"* ]]; then
  numThreads=$(sysctl -n hw.logicalcpu)
else
  cat << EOF
WARNING: Unsupported OSTYPE: cannot determine number of host CPUs"
  Defaulting to 2 threads. Use --threads N to use N threads"
EOF
  numThreads=2
fi
cmakeOptions=()
cleanBefore=no
compiler=gcc
useBazel=no
noGui=no

_help() {
    cat <<EOF
usage: $0 [OPTIONS]

OPTIONS:
  -cmake='-<key>=<value> [-<key>=<value> ...]'  User defined cmake options
                                                 Note: use single quote after
                                                 -cmake= and double quotes if
                                                 <key> has multiple <values>
                                                 e.g.: -cmake='-DFLAGS="-a -b"'
  -compiler=COMPILER_NAME                        Compiler name: gcc or clang
                                                 Default: gcc
                                                 (macOS only; on Linux the
                                                 bazel-provided clang toolchain
                                                 is always used)
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
  -cpp20                                        Use C++20 standard
  -build-man                                    Build Man Pages (optional)
  -threads=NUM_THREADS                          Number of threads to use during
                                                 compile. Default: \`nproc\` on linux
                                                 or \`sysctl -n hw.logicalcpu\` on macOS
  -keep-log                                     Keep a compile log in build dir
  -help                                         Shows this message
  -gpu                                          Enable GPU to accelerate the process
  -bazel                                        Use Bazel instead of CMake to build
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
            noGui=yes
            ;;
        -no-tests)
            cmakeOptions+=("-DENABLE_TESTS=OFF")
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
            cmakeOptions+=("-DCMAKE_BUILD_TYPE=Debug")
            cmakeOptions+=("-DCMAKE_C_FLAGS=--coverage")
            cmakeOptions+=("-DCMAKE_CXX_FLAGS=--coverage")
            cmakeOptions+=("-DCMAKE_EXE_LINKER_FLAGS=--coverage")
            ;;
        -cmake=*)
            # Use xargs to safely parse the quoted string into array elements without eval
            while IFS= read -r arg; do
                if [[ -n "$arg" ]]; then
                    cmakeOptions+=("$arg")
                fi
            done < <(echo "${1#*=}" | xargs printf '%s\n')
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
        -compiler | -cmake | -dir | -threads | -install )
            echo "${1} requires an argument" >&2
            _help
            ;;
        -gpu)
            cmakeOptions+=("-DGPU=ON")
            ;;
        -bazel)
            useBazel=yes
            ;;
        *)
            echo "unknown option: ${1}" >&2
            _help
            ;;
    esac
    shift 1
done

if [[ "$OSTYPE" == "darwin"* ]]; then
    case "${compiler}" in
        "gcc" )
            export CC="$(command -v gcc)"
            export CXX="$(command -v g++)"
            ;;
        "clang" )
            export CC="$(command -v clang)"
            export CXX="$(command -v clang++)"
            ;;
        *)
            export CC=""
            export CXX=""
    esac

    if [[ -z "${CC}" || -z "${CXX}" ]]; then
        echo "Compiler $compiler not installed or it is not supported." >&2
        _help 1
    fi
elif [[ "${compiler}" != "gcc" ]]; then
    echo "[WARNING] -compiler is ignored on Linux; the bazel-provided clang toolchain is used."
fi

if [[ "${cleanBefore}" == "yes" ]]; then
    rm -rf "${buildDir}"
fi

mkdir -p "${buildDir}"
__logging

if [[ "$OSTYPE" == "darwin"* ]]; then

    _bison=$(brew --prefix bison 2>/dev/null || true)
    _flex=$(brew --prefix flex 2>/dev/null || true)
    _ortools=$(brew --prefix or-tools 2>/dev/null || true)

    if [[ -z "$_bison" || ! -d "$_bison/bin" ]]; then
        echo "[ERROR] bison not found or broken. Run: brew install bison" >&2
        exit 1
    fi
    if [[ -z "$_flex" || ! -d "$_flex/bin" ]]; then
        echo "[ERROR] flex not found or broken. Run: brew install flex" >&2
        exit 1
    fi
    if [[ -z "$_ortools" || ! -d "$_ortools/lib" || ! -d "$_ortools/include" ]]; then
        echo "[ERROR] or-tools not found or broken. Run: brew install or-tools" >&2
        exit 1
    fi

    export PATH="$_bison/bin:$_flex/bin:$PATH"
    export CMAKE_PREFIX_PATH="${_ortools}"

    _qt5=$(brew --prefix qt@5 2>/dev/null || true)
    if [[ -z "$_qt5" || ! -d "$_qt5/lib" ]]; then
        echo "[ERROR] qt@5 not found or broken. Run: brew install qt@5" >&2
        exit 1
    fi
    
    cmakeOptions+=" -DQt5_DIR=$_qt5/lib/cmake/Qt5"

    _tcl8=$(brew --prefix tcl-tk@8 2>/dev/null || true)     
    if [[ -z "$_tcl8" || ! -d "$_tcl8/lib" || ! -d "$_tcl8/include" ]]; then
        echo "[ERROR] tcl-tk@8 not found or broken. Run: brew install tcl-tk@8" >&2
        exit 1
    fi
    
    cmakeOptions+=" -DTCL_LIBRARY=$_tcl8/lib/libtcl8.6.dylib"

    cmakeOptions+=" -DTCL_INCLUDE_PATH=$_tcl8/include"
    cmakeOptions+=" -DFLEX_INCLUDE_DIR=$_flex/include"

    cmakeOptions+=" -DCMAKE_CXX_FLAGS=-DBOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED"

    _icu="$(brew --prefix icu4c 2>/dev/null || true)"
    if [[ -z "$_icu" || ! -d "$_icu/lib" ]]; then
        echo "[ERROR] icu4c not found or broken. Run: brew install icu4c" >&2
        exit 1
    fi

    export LDFLAGS="-L$_icu/lib"
    export CPPFLAGS="-I$_icu/include" 
    export PKG_CONFIG_PATH="$_icu/lib/pkgconfig"

    _extra_lib_paths=("$(brew --prefix)/lib")

    _joined_paths="$(IFS=:; echo "${_extra_lib_paths[*]}")"

    export LIBRARY_PATH="${_joined_paths}${LIBRARY_PATH:+:$LIBRARY_PATH}"
    echo "[INFO] General LIBRARY_PATH=$LIBRARY_PATH"
fi

# ==============================================================================
# PRE-COMPILATION SYSTEM CHECKS
# ==============================================================================
if [[ -t 1 ]]; then
    RED=$(tput setaf 1)
    GREEN=$(tput setaf 2)
    YELLOW=$(tput setaf 3)
    NC=$(tput sgr0) # No Color
else
    RED=''
    GREEN=''
    YELLOW=''
    NC=''
fi

echo -e "${YELLOW}Running pre-compilation system checks...${NC}"

check_command() {
    if ! command -v "$1" &> /dev/null; then
        echo -e "${RED}[ERROR] Required dependency '$1' is missing!${NC}"
        echo "Please install it using 'sudo ./etc/DependencyInstaller.sh' before building."
        exit 1
    else
        echo -e "${GREEN}[OK] Found $1${NC}"
    fi
}

# Essential build tools required for OpenROAD. On Linux, everything —
# cmake and ninja included — comes from the bazel-materialized deps/
# prefix; only macOS uses host tools.
if [[ "$OSTYPE" == "darwin"* ]]; then
    check_command "cmake"
    check_command "bison"
    check_command "flex"
    check_command "swig"

    # Compiler check based on user selection
    if [[ "${compiler:-gcc}" == "gcc" ]]; then
        check_command "gcc"
        check_command "g++"
    elif [[ "${compiler}" == "clang" ]]; then
        check_command "clang"
        check_command "clang++"
    else
        # Handle unknown compilers gracefully - suggested by gemini-bot
        echo -e "${YELLOW}[WARNING] Unsupported compiler '${compiler}' specified. Skipping compiler pre-compilation check.${NC}"
    fi
fi

echo -e "${GREEN}All pre-compilation checks passed! Proceeding...${NC}\n"
# ==============================================================================

echo "[INFO] Using ${numThreads} threads."
if [[ "$useBazel" == "yes" ]]; then
    echo "[INFO] Building with Bazel."
    bazel_cmd="bazel"
    if command -v bazelisk &> /dev/null; then
        bazel_cmd="bazelisk"
    fi
    bazelArgs=("--config=opt" "--jobs=${numThreads}")
    if [[ "$noGui" == "yes" ]]; then
        bazelArgs+=("--//:platform=cli")
    else
        bazelArgs+=("--//:platform=gui")
    fi
    "${bazel_cmd}" build "${bazelArgs[@]}" //:openroad
    exit 0
fi
if [[ "$OSTYPE" == "linux"* && "$OSTYPE" != "linux-gnu"* ]]; then
    # The deps/ prefix (cmake, ninja, the toolchain) is glibc binaries.
    echo "[ERROR] Only glibc-based Linux (linux-gnu) is supported; got OSTYPE=$OSTYPE." >&2
    exit 1
fi
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if [[ "$(uname -m)" != "x86_64" ]]; then
        echo "[ERROR] The bazel-materialized dependency prefix only supports Linux x86_64." >&2
        exit 1
    fi
    # Refresh with: bazelisk run //:cmake
    if [[ ! -f "deps/toolchain.cmake" ]]; then
        bazel_cmd="bazel"
        if command -v bazelisk &> /dev/null; then
            bazel_cmd="bazelisk"
        fi
        echo "[INFO] Materializing CMake dependencies: ${bazel_cmd} run //:cmake"
        "${bazel_cmd}" run //:cmake
    fi
    # The bundled cmake/ctest/ninja take precedence over host tools.
    export PATH="${PWD}/deps/bin:${PATH}"
    cmakeOptions+=("-DCMAKE_TOOLCHAIN_FILE=${PWD}/deps/toolchain.cmake")
    cmakeOptions+=("-G" "Ninja")
fi
cmake "${cmakeOptions[@]}" -B "${buildDir}" .
time cmake --build "${buildDir}" -j "${numThreads}"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Self-locating wrapper: sets up the bundled Tcl/Python runtimes of the
    # deps/ prefix relative to the tree it sits in, so a stashed/copied
    # build+deps pair works on any host path. The bin -> repo root hop is
    # computed at generation time so custom -dir build directories work.
    rel_root="$(realpath --relative-to="${buildDir}/bin" .)"
    cat > "${buildDir}/bin/openroad-wrapper" <<EOF
#!/bin/sh
d="\$(dirname "\$(readlink -f "\$0")")"
root="\$d/${rel_root}"
export TCL_LIBRARY="\$root/deps/lib/tcl9.0"
export PYTHONHOME="\$root/deps/python"
export LD_LIBRARY_PATH="\$root/deps/python/lib\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}"
exec "\$d/openroad" "\$@"
EOF
    chmod +x "${buildDir}/bin/openroad-wrapper"
    echo "[INFO] Runtime environment for the built binary:"
    echo "[INFO]   export TCL_LIBRARY=${PWD}/deps/lib/tcl9.0"
    echo "[INFO]   export PYTHONHOME=${PWD}/deps/python"
    echo "[INFO] or use ${buildDir}/bin/openroad-wrapper"
fi
