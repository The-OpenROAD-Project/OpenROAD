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
depsPrefixesFile=""
compiler=gcc
compilerSet=no
useBazel=yes
bazelGpu=no
noGui=no
installPrefix=""
# Flags given that only the CMake build uses; warned about after parsing,
# once we know whether -cmake-build came later on the command line.
bazelIgnoredFlags=()

_help() {
    cat <<EOF
usage: $0 [OPTIONS]

Build and install OpenROAD. Bazel is the default build system and puts the
binary in bazel-bin/openroad. CMake is deprecated: select it with -cmake-build.

OPTIONS:
  -no-gui                                       Disable GUI support
  -gpu                                          Build the opt-in GPU (Kokkos/CUDA)
                                                 placement backends: --config=gpu
                                                 with Bazel, -DENABLE_GPU=ON with CMake.
  -threads=NUM_THREADS                          Number of threads to use during
                                                 compile. Default: \`nproc\` on linux
                                                 or \`sysctl -n hw.logicalcpu\` on macOS
  -local                                        Install OpenROAD in \${HOME}/.local.
  -prefix=DIR                                   Install OpenROAD in a user-specified directory.
  -dir=PATH                                     Directory for the build log, which is
                                                 always saved there as
                                                 openroad_build.log. With CMake it also
                                                 holds the build files.
                                                 Default: ./build
  -clean                                        Remove the -dir directory before the
                                                 build. With Bazel this does not clear
                                                 the Bazel cache: use 'bazelisk clean'.
  -cpp20                                        DEPRECATED: no-op, C++20 is the default
  -help                                         Shows this message

CMAKE ONLY (deprecated; with Bazel these print a warning and have no effect):
  -cmake-build                                  DEPRECATED: Force the CMake build instead
                                                 of Bazel. Bazel is the supported build
                                                 system and is used by default; CMake
                                                 support will be removed on Nov. 1,
                                                 2026.
  -cmake='-<key>=<value> [-<key>=<value> ...]'  User defined cmake options
                                                 Note: use single quote after
                                                 -cmake= and double quotes if
                                                 <key> has multiple <values>
                                                 e.g.: -cmake='-DFLAGS="-a -b"'
  -compiler=COMPILER_NAME                        Compiler name: gcc or clang
                                                 Default: gcc
  -no-warnings
                                                Compiler warnings are
                                                considered errors, i.e.,
                                                use -Werror flag during build.
  -no-tests                                     Disable GTest
  -coverage                                     Enable cmake coverage options
  -build-man                                    Build Man Pages (optional).
                                                 Bazel builds them with
                                                 //docs:man_pages.
  -deps-prefixes-file=FILE                      File with CMake packages roots,
                                                 its content extends -cmake argument.
                                                 By default, "openroad_deps_prefixes.txt"
                                                 file from OpenROAD's "etc" directory
                                                 or from system "/etc".

EXAMPLES:
  $0                                    Build and install with Bazel (default)
  $0 -no-gui -prefix=/opt/openroad      CLI-only build, installed in /opt/openroad
  $0 -cmake-build                       Build with CMake (deprecated)

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
            installPrefix="${HOME}/.local"
            INSTALL_PREFIX_SET=1
            ;;
        -prefix=*)
            if [[ -n "${INSTALL_PREFIX_SET:-}" ]]; then
                echo "[WARNING] Previous -prefix or -local argument will be overwritten." >&2
            fi
            cmakeOptions+=("-DCMAKE_INSTALL_PREFIX=${1#*=}")
            installPrefix="${1#*=}"
            # bazel run //:install resolves its argument from the runfiles dir,
            # not the workspace, so a relative prefix must be made absolute.
            if [[ "${installPrefix}" != /* ]]; then
                installPrefix="${PWD}/${installPrefix}"
            fi
            INSTALL_PREFIX_SET=1
            ;;
        -no-gui)
            cmakeOptions+=("-DBUILD_GUI=OFF")
            noGui=yes
            ;;
        -no-tests)
            bazelIgnoredFlags+=("-no-tests")
            cmakeOptions+=("-DENABLE_TESTS=OFF")
            ;;
        -cpp20)
            echo "[WARNING] -cpp20 is deprecated and ignored: C++20 is already the default." >&2
            ;;
        -build-man)
            bazelIgnoredFlags+=("-build-man")
            cmakeOptions+=("-DBUILD_MAN=ON")
            ;;
        -compiler=*)
            compiler="${1#*=}"
            compilerSet=yes
            ;;
        -no-warnings )
            bazelIgnoredFlags+=("-no-warnings")
            cmakeOptions+=("-DALLOW_WARNINGS=OFF")
            ;;
        -coverage )
            bazelIgnoredFlags+=("-coverage")
            cmakeOptions+=("-DCMAKE_BUILD_TYPE=Debug")
            cmakeOptions+=("-DCMAKE_CXX_FLAGS=-fprofile-arcs -ftest-coverage")
            cmakeOptions+=("-DCMAKE_EXE_LINKER_FLAGS=-lgcov")
            ;;
        -cmake=*)
            bazelIgnoredFlags+=("-cmake=")
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
        -deps-prefixes-file=*)
            file="${1#-deps-prefixes-file=}"
            if [[ ! -f "$file" ]]; then 
                echo "${file} does not exist" >&2
                _help
            fi
            depsPrefixesFile="$file"
            bazelIgnoredFlags+=("-deps-prefixes-file=")
            ;;
        -compiler | -cmake | -dir | -threads | -install | -deps-prefixes-file )
            echo "${1} requires an argument" >&2
            _help
            ;;
        -gpu)
            cmakeOptions+=("-DENABLE_GPU=ON")
            bazelGpu=yes
            ;;
        -cmake-build)
            echo "[WARNING] -cmake-build selects the deprecated CMake build: Bazel is the supported build system and CMake support will be removed on Nov. 1, 2026." >&2
            useBazel=no
            ;;
        -bazel)
            echo "[WARNING] -bazel is deprecated: Bazel is now the default build system." >&2
            useBazel=yes
            ;;
        *)
            echo "unknown option: ${1}" >&2
            _help
            ;;
    esac
    shift 1
done

# Bazel selects its own compiler toolchain and dependencies, so the -compiler
# flag, the deps prefixes file, and the CC/CXX resolution below only apply to
# CMake builds.
if [[ "$useBazel" == "yes" ]]; then
    if [[ "$compilerSet" == "yes" ]]; then
        echo "[WARNING] -compiler is ignored with Bazel: Bazel manages its own toolchain." >&2
    fi
    warnedFlags=" "
    # ${arr[@]+...} keeps an empty array safe under set -u on bash < 4.4.
    for flag in ${bazelIgnoredFlags[@]+"${bazelIgnoredFlags[@]}"}; do
        # Warn once per flag, even when it is repeated on the command line.
        case "$warnedFlags" in
            *" ${flag} "*) continue ;;
        esac
        warnedFlags="${warnedFlags}${flag} "
        echo "[WARNING] ${flag} is ignored with Bazel: it only applies to the deprecated CMake build (-cmake-build)." >&2
    done
else
    if [[ -z "$depsPrefixesFile" ]]; then
        if [[ -f "$DIR/openroad_deps_prefixes.txt" ]]; then
            depsPrefixesFile="$DIR/openroad_deps_prefixes.txt"
        elif [[ -f "/etc/openroad_deps_prefixes.txt" ]]; then
            depsPrefixesFile="/etc/openroad_deps_prefixes.txt"
        fi
    fi
    if [[ -f "$depsPrefixesFile" ]]; then
        while IFS= read -r arg; do
            if [[ -n "$arg" ]]; then
                cmakeOptions+=("$arg")
            fi
        done < <(xargs printf '%s\n' < "$depsPrefixesFile")
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
fi

if [[ "${cleanBefore}" == "yes" ]]; then
    rm -rf "${buildDir}"
fi

mkdir -p "${buildDir}"
__logging

if [[ "$OSTYPE" == "darwin"* && "$useBazel" == "no" ]]; then

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
        echo "Please install it using 'sudo ./etc/DependencyInstaller.sh -cmake' before building."
        exit 1
    else
        echo -e "${GREEN}[OK] Found $1${NC}"
    fi
}

if [[ "$useBazel" == "yes" ]]; then
    # Bazel drives its own toolchain and dependencies, so only the launcher
    # (bazelisk preferred, bazel as fallback) needs to be present.
    if command -v bazelisk &> /dev/null; then
        echo -e "${GREEN}[OK] Found bazelisk${NC}"
    elif command -v bazel &> /dev/null; then
        echo -e "${GREEN}[OK] Found bazel${NC}"
    else
        echo -e "${RED}[ERROR] Required dependency 'bazelisk' (or 'bazel') is missing!${NC}"
        echo "Please install it using './etc/DependencyInstaller.sh -bazel' (with sudo on Linux) before building."
        exit 1
    fi
else
    # Essential build tools required for OpenROAD
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
    elif [[ "${compiler}" == "clang-16" ]]; then
        check_command "clang-16"
        check_command "clang++-16"
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
    # Build.sh is the install path (ORFS, Docker, packaging). Plain
    # `bazel build` embeds the "bazel-nostamp" placeholder so dev builds stay
    # cacheable; --config=release turns on --stamp so `openroad -version`
    # reports the same `git describe` string the CMake build always did.
    # --config=release already includes --config=opt. Adding --config=opt
    # again repeats its flags (-O3 twice), which changes every compile
    # action key so no action hits the remote cache that CI fills.
    bazelArgs=("--jobs=${numThreads}" "--config=release")
    if [[ "$bazelGpu" == "yes" ]]; then
        bazelArgs+=("--config=gpu")
    fi
    if [[ "$noGui" == "yes" ]]; then
        bazelArgs+=("--//:platform=cli")
    else
        bazelArgs+=("--//:platform=gui")
    fi
    "${bazel_cmd}" build "${bazelArgs[@]}" //:openroad
    # Install with the same configuration the binary was built with so the
    # correct platform (gui/cli) artifacts are shipped. install.sh takes the
    # prefix as its first argument, falling back to its own default location
    # when -local/-prefix were not given.
    if [[ -n "${installPrefix}" ]]; then
        "${bazel_cmd}" run "${bazelArgs[@]}" //:install -- "${installPrefix}"
    else
        "${bazel_cmd}" run "${bazelArgs[@]}" //:install
    fi
    exit 0
fi
cmake "${cmakeOptions[@]}" -B "${buildDir}" .
time cmake --build "${buildDir}" -j "${numThreads}"
# Only install when the user asked for a prefix (-local/-prefix); the CMake
# default prefix (/usr/local) would require root.
if [[ -n "${INSTALL_PREFIX_SET:-}" ]]; then
    cmake --install "${buildDir}"
fi
