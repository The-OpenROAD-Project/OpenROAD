#!/usr/bin/env bash

# Strict Mode
set -euo pipefail

# ==============================================================================
# Configuration
# ==============================================================================

# ------------------------------------------------------------------------------
# Script Configuration
# ------------------------------------------------------------------------------
# Default values for command-line arguments
PREFIX=""
CI="no"
BASE_DIR=$(mktemp -d /tmp/DependencyInstaller-XXXXXX)
INSTALL_SUMMARY=()
VERBOSE_MODE="no"
OPT_NOCERT=""
INSTALL_BAZEL_DEV="no"
NO_GUI="no"

# Colors
if [[ -t 1 ]]; then
    export RED=$(tput setaf 1)
    export GREEN=$(tput setaf 2)
    export YELLOW=$(tput setaf 3)
    export BLUE=$(tput setaf 4)
    export BOLD=$(tput bold)
    export NC=$(tput sgr0) # No Color
else
    RED=""
    GREEN=""
    YELLOW=""
    BLUE=""
    BOLD=""
    NC=""
fi

# ------------------------------------------------------------------------------
# Dependency Versions and Checksums
# ------------------------------------------------------------------------------
BAZELISK_VERSION="1.28.1"
BAZELISK_CHECKSUM_AMD64="2dc74b7ad6bdd6b6b08f6802d14fc1fd"
BAZELISK_CHECKSUM_ARM64="94415d08ed2f86a49375f25a7f2f9cca"
BUILDIFIER_VERSION="8.5.1"
BUILDIFIER_CHECKSUM_AMD64="72f5953ab6dcc309a4447c2e2d79c680"
BUILDIFIER_CHECKSUM_ARM64="06f52f0872bde33685c6260110261cf7"

# ==============================================================================
# Helper Functions
# ==============================================================================
# ------------------------------------------------------------------------------
# Logging and Error Handling
# ------------------------------------------------------------------------------
log() {
    echo "${BLUE}${BOLD}[INFO]${NC} $1"
}

warn() {
    echo "${YELLOW}${BOLD}[WARNING]${NC} $1" >&2
}

error() {
    echo "${RED}${BOLD}[ERROR]${NC} $1" >&2
    exit 1
}

# ------------------------------------------------------------------------------
# Version Comparison
# ------------------------------------------------------------------------------
# Compares two semantic versions.
# Usage: _version_compare "1.2.3" -ge "1.2.0"
_version_compare() {
    local op
    case "$2" in
        -ge) op=">=" ;;
        -gt) op=">" ;;
        -le) op="<=" ;;
        -lt) op="<" ;;
        -eq) op="==" ;;
        -ne) op="!=" ;;
        *)
            error "Invalid operator in _version_compare: $2"
            ;;
    esac
    awk -v v1="$1" -v v2="$3" 'BEGIN { exit !(v1 '"$op"' v2) }'
}

# ------------------------------------------------------------------------------
# Check Command Existence
# ------------------------------------------------------------------------------
# Checks if a command is available in the system's PATH.
# Usage: _command_exists "curl"
_command_exists() {
    command -v "$1" &> /dev/null
}

# ------------------------------------------------------------------------------
# Path Truncation
# ------------------------------------------------------------------------------
_truncate_path() {
    local path=$1
    local max_len=$2
    if [[ ${#path} -gt ${max_len} ]]; then
        local start_len=$(( (max_len - 3) / 2 ))
        local end_len=$(( max_len - 3 - start_len ))
        echo "${path:0:$start_len}...${path: -${end_len}}"
    else
        echo "$path"
    fi
}

# ------------------------------------------------------------------------------
# Checksum Verification
# ------------------------------------------------------------------------------
_verify_checksum() {
    local checksum=$1
    local filename=$2
    _execute "Verifying ${filename} checksum..." bash -c "echo '${checksum}  ${filename}' | md5sum --quiet -c -"
}

# ------------------------------------------------------------------------------
# Command Execution
# ------------------------------------------------------------------------------
# Execute a command, hiding output unless in verbose mode or an error occurs.
# Usage: _execute "Downloading bazelisk..." curl ...
_execute() {
    local description=$1
    shift

    if [[ "${VERBOSE_MODE}" == "yes" ]]; then
        log "${description}"
        "$@"
        return
    fi

    echo -n "${BLUE}${BOLD}[INFO]${NC} ${description}"
    local log_file
    log_file=$(mktemp)
    if ! "$@" &> "${log_file}"; then
        echo -e " ${RED}✖${NC}"
        cat "${log_file}"
        rm "${log_file}"
        error "Failed to execute: $*"
    else
        echo -e " ${GREEN}✔${NC}"
        rm "${log_file}"
    fi
}

# ==============================================================================
# Dependency Installation Modules
# ==============================================================================
# ------------------------------------------------------------------------------
# Bazel
# ------------------------------------------------------------------------------
# bazelisk and the libraries a Bazel build needs are installed independently:
# something else may already have put bazelisk on PATH, and skipping the
# libraries in that case would leave a system that has bazelisk but cannot
# link. -common installs bazelisk, -base the libraries.
_install_bazelisk() {
    local bazel_prefix=${PREFIX:-"/usr/local"}
    log "Checking Bazel (via bazelisk)"
    if _command_exists "bazelisk"; then
        log "bazelisk already installed, skipping."
        INSTALL_SUMMARY+=("Bazel: system=found, required=any, status=skipped")
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        _execute "Installing bazelisk via Homebrew..." brew install bazelisk
        INSTALL_SUMMARY+=("Bazel: system=none, required=latest, status=installed")
    else
        # curl fetches bazelisk below but is not in every base image.
        if ! _command_exists "curl"; then
            if _command_exists "apt-get"; then
                _execute "Updating package lists..." apt-get -y update
                _execute "Installing curl..." \
                    apt-get -y install --no-install-recommends curl ca-certificates
            elif _command_exists "yum"; then
                _execute "Installing curl..." yum install -y curl ca-certificates
            fi
        fi
        local arch
        arch=$(uname -m)
        local bazelisk_arch="amd64"
        if [[ "${arch}" == "aarch64" ]]; then
            bazelisk_arch="arm64"
        fi
        local bazelisk_checksum="${BAZELISK_CHECKSUM_AMD64}"
        if [[ "${bazelisk_arch}" == "arm64" ]]; then
            bazelisk_checksum="${BAZELISK_CHECKSUM_ARM64}"
        fi
        mkdir -p "${bazel_prefix}/bin"
        (
            cd "${BASE_DIR}"
            _execute "Downloading bazelisk v${BAZELISK_VERSION}..." curl ${OPT_NOCERT} -Lo bazelisk \
                "https://github.com/bazelbuild/bazelisk/releases/download/v${BAZELISK_VERSION}/bazelisk-linux-${bazelisk_arch}"
            _verify_checksum "${bazelisk_checksum}" "bazelisk" || error "Bazelisk checksum failed."
            chmod +x bazelisk
            _execute "Installing bazelisk..." mv bazelisk "${bazel_prefix}/bin/bazelisk"
        )
        INSTALL_SUMMARY+=("Bazel: system=none, required=latest, status=installed")
    fi
}

# Runtime libraries for the prebuilt LLVM toolchain and, unless -no-gui, for
# the Qt GUI binary. Homebrew resolves these itself on macOS.
_install_bazel_libs() {
    if [[ "$OSTYPE" != "darwin"* ]]; then
        if _command_exists "apt-get"; then
            # Ubuntu 26.04 ships the libxml2 runtime with soname
            # libxml2.so.16, but the prebuilt LLVM toolchain (lld) pulled in
            # by the Bazel build is linked against the old libxml2.so.2.
            # Pull in libxml2-dev there (and add a compatibility symlink
            # below); older Ubuntu still provides .so.2 via libxml2.
            local ubuntu_version=""
            if [[ -f /etc/os-release ]]; then
                ubuntu_version=$(awk -F= '/^VERSION_ID/{print $2}' /etc/os-release | sed 's/"//g')
            fi
            local libxml2_pkg="libxml2"
            if [[ -n "${ubuntu_version}" ]] && _version_compare "${ubuntu_version}" -ge "26.04"; then
                libxml2_pkg="libxml2-dev"
            fi
            _execute "Updating package lists..." apt-get -y update
            _execute "Installing bazel required libraries..." \
                apt-get -y install --no-install-recommends \
                libc6-dev "${libxml2_pkg}" libtinfo6 zlib1g libstdc++6 \
                curl ca-certificates
            # lld only uses libxml2 for Windows COFF manifests, never during a
            # Linux link, so the .so.16 -> .so.2 compatibility symlink is safe.
            # Gated to 26.04+ only.
            if [[ -n "${ubuntu_version}" ]] && _version_compare "${ubuntu_version}" -ge "26.04"; then
                local libdir="/usr/lib/$(uname -m)-linux-gnu"
                local libxml2_so
                libxml2_so=$(ls "${libdir}"/libxml2.so.* 2>/dev/null \
                    | grep -v 'libxml2.so.2$' | head -n1)
                if [[ ! -e "${libdir}/libxml2.so.2" && -n "${libxml2_so}" ]]; then
                    _execute "Adding libxml2.so.2 compatibility symlink for prebuilt LLVM lld..." \
                        ln -sf "$(basename "${libxml2_so}")" "${libdir}/libxml2.so.2"
                fi
            fi
        elif _command_exists "yum"; then
            _execute "Installing bazel required libraries..." \
                yum install -y \
                glibc-devel libxml2 ncurses-libs zlib libstdc++
            # RHEL 9 ships curl-minimal, which conflicts with the full curl
            # package, so install curl only when no curl is present.
            if ! _command_exists "curl"; then
                _execute "Installing curl..." yum install -y curl ca-certificates
            fi
        fi
        if [[ "${NO_GUI}" != "yes" ]]; then
            # Runtime xcb libraries only, no -dev/-devel: Qt comes from the
            # qt-bazel prebuilts, so nothing compiles against system xcb
            # headers. The GUI binary resolves these at load time, which is the
            # whole reason they are here. The -devel variants also live in
            # PowerTools/CRB on RHEL-likes, which is not enabled by default.
            if _command_exists "apt-get"; then
                _execute "Installing xcb libraries for GUI support..." \
                    apt-get -y install --no-install-recommends \
                    libx11-6 libx11-xcb1 libsm6 libice6 \
                    libxcb1 libxcb-cursor0 libxcb-icccm4 libxcb-image0 \
                    libxcb-keysyms1 libxcb-randr0 libxcb-render0 \
                    libxcb-render-util0 libxcb-shape0 libxcb-shm0 libxcb-sync1 \
                    libxcb-util1 libxcb-xfixes0 libxcb-xinerama0 libxcb-xkb1 \
                    libdbus-1-3 libfontconfig1 libxkbcommon0 libxkbcommon-x11-0
            elif _command_exists "yum"; then
                _execute "Installing xcb libraries for GUI support..." \
                    yum install -y \
                    libxcb xcb-util xcb-util-image xcb-util-keysyms \
                    xcb-util-renderutil xcb-util-wm \
                    libX11-xcb libX11 libSM libICE \
                    dbus-libs fontconfig \
                    libxkbcommon libxkbcommon-x11
                # On RHEL 8 xcb-util-cursor comes from EPEL rather than the
                # default repositories (9 ships it in AppStream), so it may be
                # missing on a bare image.
                # Probe rather than hard-fail: without it the GUI binary cannot
                # open a window, but the cli binary links and runs fine.
                if yum -q info xcb-util-cursor > /dev/null 2>&1; then
                    _execute "Installing xcb-util-cursor..." yum install -y xcb-util-cursor
                else
                    log "xcb-util-cursor unavailable on this release; skipping (the GUI binary will not run here)."
                fi
            fi
        fi
    fi
}

# ------------------------------------------------------------------------------
# Bazel Dev Tools (buildifier, etc.)
# ------------------------------------------------------------------------------
_install_bazel_dev() {
    local bazel_prefix=${PREFIX:-"/usr/local"}
    log "Checking Bazel dev tools (buildifier)"
    if _command_exists "buildifier"; then
        log "buildifier already installed, skipping."
        INSTALL_SUMMARY+=("buildifier: system=found, required=any, status=skipped")
        return
    fi
    if [[ "$OSTYPE" == "darwin"* ]]; then
        _execute "Installing buildifier via Homebrew..." brew install buildifier
    else
        local arch
        arch=$(uname -m)
        local buildifier_arch="amd64"
        if [[ "${arch}" == "aarch64" ]]; then
            buildifier_arch="arm64"
        fi
        local buildifier_checksum="${BUILDIFIER_CHECKSUM_AMD64}"
        if [[ "${buildifier_arch}" == "arm64" ]]; then
            buildifier_checksum="${BUILDIFIER_CHECKSUM_ARM64}"
        fi
        mkdir -p "${bazel_prefix}/bin"
        (
            cd "${BASE_DIR}"
            _execute "Downloading buildifier v${BUILDIFIER_VERSION}..." curl ${OPT_NOCERT} -Lo buildifier \
                "https://github.com/bazelbuild/buildtools/releases/download/v${BUILDIFIER_VERSION}/buildifier-linux-${buildifier_arch}"
            _verify_checksum "${buildifier_checksum}" "buildifier" || error "Buildifier checksum failed."
            chmod +x buildifier
            _execute "Installing buildifier..." mv buildifier "${bazel_prefix}/bin/buildifier"
        )
    fi
    INSTALL_SUMMARY+=("buildifier: system=none, required=latest, status=installed")
}

# ------------------------------------------------------------------------------
# CI Dependencies (Ubuntu only)
# ------------------------------------------------------------------------------
_install_ci_packages() {
    log "Install CI dependencies (-ci)"
    export DEBIAN_FRONTEND="noninteractive"
    _execute "Updating package lists..." apt-get -y update
    _execute "Installing CI packages..." apt-get -y install --no-install-recommends \
        apt-transport-https ca-certificates curl git gnupg jq lsb-release parallel \
        python3 python3-pandas python3-pip software-properties-common \
        time unzip zip

    if _command_exists "docker"; then
        log "Docker is already installed, skipping reinstallation."
        return 0
    fi

    _execute "Creating Docker keyring directory..." install -m 0755 -d /etc/apt/keyrings
    _execute "Downloading Docker GPG key..." curl ${OPT_NOCERT} -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
    chmod a+r /etc/apt/keyrings/docker.asc
    _execute "Adding Docker repository..." bash -c 'echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
        $(. /etc/os-release && echo \"${VERSION_CODENAME}\") stable" | tee /etc/apt/sources.list.d/docker.list > /dev/null'
    _execute "Updating package lists for Docker..." apt-get -y update
    _execute "Installing Docker..." apt-get -y install --no-install-recommends docker-ce docker-ce-cli containerd.io docker-buildx-plugin
}

_help() {
    cat <<EOF

Usage: $0 [OPTIONS]

Install the dependencies for the Bazel build of OpenROAD.

Scope (default: -all):
  -all                        Install base and common dependencies. Requires privileged access.
  -base                       Install the runtime libraries the Bazel build needs using the
                              system package manager. Requires privileged access.
  -common                     Install bazelisk into the prefix.

Other options:
  -bazel                      Accepted for compatibility: Bazel dependencies are always installed.
  -bazel-dev                  Download and install bazel developer tools (buildifier, etc.).
  -no-gui                     Skip GUI-only dependencies (e.g. xcb libraries).
  -prefix=DIR                 Install common dependencies in a user-specified directory.
  -local                      Install common dependencies in \${HOME}/.local.
  -ci                         Install dependencies required for CI (Ubuntu only).
  -nocert                     Disable certificate checks for downloads.
  -constant-build-dir         Use a constant download directory instead of a random one.
  -verbose                    Show all output from install commands.
  -h, -help                   Show this help message.

EOF
    exit "${1:-1}"
}

main() {
    local option="none"
    while [ "$#" -gt 0 ]; do
        case "${1}" in
            -h|-help) _help 0 ;;
            -all) option="all" ;;
            -base) option="base" ;;
            -common) option="common" ;;
            -bazel) ;;
            -cmake)
                error "-cmake: the CMake build was removed. OpenROAD builds with Bazel: see docs/user/Bazel.md."
                ;;
            -bazel-dev) INSTALL_BAZEL_DEV="yes" ;;
            -no-gui) NO_GUI="yes" ;;
            -ci) CI="yes" ;;
            -verbose) VERBOSE_MODE="yes" ;;
            -local)
                if [[ $(id -u) == 0 ]]; then
                    error "Cannot use -local with root or sudo."
                fi
                if [[ -n "${PREFIX}" ]]; then
                    warn "Previous -prefix argument will be overwritten with -local."
                fi
                PREFIX="${HOME}/.local"
                ;;
            -constant-build-dir)
                if [[ -d "${BASE_DIR}" ]]; then
                    log "Removing old building directory ${BASE_DIR}"
                    rm -r "${BASE_DIR}"
                fi
                BASE_DIR="/tmp/DependencyInstaller-OpenROAD"
                mkdir -p "${BASE_DIR}"
                ;;
            -prefix=*)
                if [[ -n "${PREFIX}" ]]; then
                    warn "Previous -local argument will be overwritten with -prefix."
                fi
                PREFIX="${1#*=}"
                if [[ ! "${PREFIX}" = /* ]]; then
                    PREFIX="$(pwd)/${PREFIX}"
                fi
                ;;
            -nocert)
                warn "Security certificates for downloaded packages will not be checked."
                OPT_NOCERT="--insecure"
                ;;
            # Bazel fetches these dependencies itself, so the options that
            # selected them have no effect.
            -eqy | -skip-system-or-tools | -save-deps-prefixes=* | -threads=* | -yosys-ver=*)
                warn "${1%%=*} has no effect: the Bazel build does not use it."
                ;;
            *)
                echo "Unknown option: ${1}" >&2
                _help
                ;;
        esac
        shift 1
    done

    if [[ "${option}" == "none" ]]; then
        option="all"
    fi

    if [[ "${CI}" == "yes" ]]; then
        local os=""
        if [[ -f /etc/os-release ]]; then
            os=$(awk -F= '/^NAME/{print $2}' /etc/os-release | sed 's/"//g')
        fi
        if [[ "${os}" == "Ubuntu" ]]; then
            _install_ci_packages
        else
            warn "-ci only applies to Ubuntu; skipping the CI dependencies."
        fi
    fi

    if [[ "${option}" == "base" || "${option}" == "all" ]]; then
        _install_bazel_libs
    fi
    if [[ "${option}" == "common" || "${option}" == "all" ]]; then
        _install_bazelisk
    fi

    if [[ "${INSTALL_BAZEL_DEV}" == "yes" ]]; then
        _install_bazel_dev
    fi

    _print_summary
    rm -rf "${BASE_DIR}"
}

_print_summary() {
    if [ ${#INSTALL_SUMMARY[@]} -eq 0 ]; then
        return
    fi
    echo ""
    log "Installation Summary"
    echo "${BOLD}=============================================================================================${NC}"
    printf "%-15s | %-25s | %-15s | %-15s | %-10s\n" "Package" "Path" "Found" "Required" "Status"
    printf "%-15s | %-25s | %-15s | %-15s | %-10s\n" "---------------" "-------------------------" "---------------" "---------------" "----------"
    for summary_line in "${INSTALL_SUMMARY[@]}"; do
        # summary_line is like "Bazel: system=found, required=any, status=skipped"
        local package
        package=$(echo "$summary_line" | cut -d':' -f1)
        local system_version
        system_version=$(echo "$summary_line" | sed -n 's/.*system=\([^,]*\).*/\1/p')
        local required_version
        required_version=$(echo "$summary_line" | sed -n 's/.*required=\([^,]*\).*/\1/p')
        local path
        path=$(echo "$summary_line" | sed -n 's/.*path=\([^,]*\).*/\1/p')
        local truncated_path
        truncated_path=$(_truncate_path "$path" 25)
        local status
        status=$(echo "$summary_line" | sed -n 's/.*status=\([^,]*\).*/\1/p')
        local status_color=""
        if [[ "${status}" == "installed" ]]; then
            status_color="${GREEN}"
        elif [[ "${status}" == "skipped" ]]; then
            status_color="${YELLOW}"
        fi
        printf "%-15s | %-25s | %-15s | %-15s | ${status_color}%-10s${NC}\n" "$package" "$truncated_path" "$system_version" "$required_version" "$status"
    done
    echo "${BOLD}=============================================================================================${NC}"
}

main "$@"
