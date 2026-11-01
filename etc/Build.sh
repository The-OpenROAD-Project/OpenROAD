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
  Defaulting to 2 threads. Use -threads=N to use N threads"
EOF
  numThreads=2
fi
cleanBefore=no
bazelGpu=no
noGui=no
installPrefix=""

_help() {
    cat <<EOF
usage: $0 [OPTIONS]

Build and install OpenROAD with Bazel. The binary is built in
bazel-bin/openroad.

OPTIONS:
  -no-gui                   Disable GUI support
  -gpu                      Build the opt-in GPU (Kokkos/CUDA) placement
                             backends (--config=gpu).
  -threads=NUM_THREADS      Number of threads to use during compile.
                             Default: \`nproc\` on linux or
                             \`sysctl -n hw.logicalcpu\` on macOS
  -local                    Install OpenROAD in \${HOME}/.local.
  -prefix=DIR               Install OpenROAD in a user-specified directory.
  -dir=PATH                 Directory for the build log, which is saved
                             there as openroad_build.log. Default: ./build
  -clean                    Remove the -dir directory before the build.
                             This does not clear the Bazel cache: use
                             'bazelisk clean'.
  -cpp20                    DEPRECATED: no-op, C++20 is the default
  -help                     Shows this message

EXAMPLES:
  $0                                    Build and install
  $0 -no-gui -prefix=/opt/openroad      CLI-only build, installed in /opt/openroad

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
            installPrefix="${HOME}/.local"
            INSTALL_PREFIX_SET=1
            ;;
        -prefix=*)
            if [[ -n "${INSTALL_PREFIX_SET:-}" ]]; then
                echo "[WARNING] Previous -prefix or -local argument will be overwritten." >&2
            fi
            installPrefix="${1#*=}"
            # bazel run //:install resolves its argument from the runfiles dir,
            # not the workspace, so a relative prefix must be made absolute.
            if [[ "${installPrefix}" != /* ]]; then
                installPrefix="${PWD}/${installPrefix}"
            fi
            INSTALL_PREFIX_SET=1
            ;;
        -no-gui)
            noGui=yes
            ;;
        -cpp20)
            echo "[WARNING] -cpp20 is deprecated and ignored: C++20 is already the default." >&2
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
        -dir | -threads )
            echo "${1} requires an argument" >&2
            _help
            ;;
        -gpu)
            bazelGpu=yes
            ;;
        -cmake-build | -cmake | -cmake=* )
            echo "[ERROR] ${1%%=*}: the CMake build was removed. OpenROAD builds with Bazel: see docs/user/Bazel.md." >&2
            exit 1
            ;;
        -bazel)
            echo "[WARNING] -bazel is deprecated: Bazel is the only build system." >&2
            ;;
        *)
            echo "unknown option: ${1}" >&2
            _help
            ;;
    esac
    shift 1
done

if [[ "${cleanBefore}" == "yes" ]]; then
    rm -rf "${buildDir}"
fi

mkdir -p "${buildDir}"
__logging

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

# Bazel drives its own toolchain and dependencies, so only the launcher
# (bazelisk preferred, bazel as fallback) needs to be present.
if command -v bazelisk &> /dev/null; then
    echo -e "${GREEN}[OK] Found bazelisk${NC}"
    bazel_cmd="bazelisk"
elif command -v bazel &> /dev/null; then
    echo -e "${GREEN}[OK] Found bazel${NC}"
    bazel_cmd="bazel"
else
    echo -e "${RED}[ERROR] Required dependency 'bazelisk' (or 'bazel') is missing!${NC}"
    echo "Please install it using './etc/DependencyInstaller.sh -bazel' (with sudo on Linux) before building."
    exit 1
fi

echo -e "${GREEN}All pre-compilation checks passed! Proceeding...${NC}\n"
# ==============================================================================

echo "[INFO] Using ${numThreads} threads."
# Build.sh is the install path (ORFS, Docker, packaging). Plain
# `bazel build` embeds the "bazel-nostamp" placeholder so dev builds stay
# cacheable; --config=release turns on --stamp so `openroad -version`
# reports the `git describe` string.
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
