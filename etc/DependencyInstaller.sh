#!/usr/bin/env bash

# Strict Mode
set -euo pipefail

# ==============================================================================
# Configuration
# ==============================================================================
# All dependency versions, checksums, and installation prefixes will be
# defined here to provide a single, easy-to-manage location for configuration.

# ------------------------------------------------------------------------------
# Script Configuration
# ------------------------------------------------------------------------------
# Default values for command-line arguments
PREFIX=""
CI="no"
NUM_THREADS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
BASE_DIR=$(mktemp -d /tmp/DependencyInstaller-XXXXXX)
INSTALL_SUMMARY=()
VERBOSE_MODE="no"
OPT_NOCERT=""

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
# C++ build dependencies (boost, or-tools, swig, bison, flex, eigen, ...)
# are not installed by this script: they come pinned from the bazel module
# graph (MODULE.bazel), for both the bazel build and plain CMake builds via
# `bazelisk run //:cmake` (see docs/user/Build.md).
YOSYS_VERSION="v0.58"
EQUIVALENCE_DEPS="no"
INSTALL_BAZEL="no"
INSTALL_BAZEL_DEV="no"
NO_GUI="no"
BAZELISK_VERSION="1.28.1"
BAZELISK_CHECKSUM_AMD64="2dc74b7ad6bdd6b6b08f6802d14fc1fd"
BAZELISK_CHECKSUM_ARM64="94415d08ed2f86a49375f25a7f2f9cca"
BUILDIFIER_VERSION="8.5.1"
BUILDIFIER_CHECKSUM_AMD64="72f5953ab6dcc309a4447c2e2d79c680"
BUILDIFIER_CHECKSUM_ARM64="06f52f0872bde33685c6260110261cf7"
# ... configuration variables will be added here ...

# ==============================================================================
# Helper Functions
# ==============================================================================
# This section will contain generic helper functions for version comparison,
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
# Usage: _version_compare "1.2.3" ">=" "1.2.0"
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
# Usage: _command_exists "cmake"
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
# Yosys
# ------------------------------------------------------------------------------
# Note: yosys's compile-time readline dependency (libreadline-dev /
# readline-devel / readline brew formula) is installed in the per-platform
# -base package functions below. It used to live in a separate helper invoked
# from here, but that put a root-only apt-get inside the unprivileged -common
# phase (see ORFS issue #4266).
_install_yosys() {
    local yosys_prefix=${PREFIX:-"/usr/local"}
    local yosys_bin=${yosys_prefix}/bin/yosys
    local yosys_installed_version="none"
    if [[ -f "${yosys_bin}" ]]; then
        yosys_installed_version=$("${yosys_bin}" --version | awk '{print $2}')
    elif _command_exists "yosys" && [[ -z "${PREFIX}" ]]; then
        yosys_installed_version=$(yosys --version | awk '{print $2}')
    fi

    local required_version="${YOSYS_VERSION#v}"
    log "Checking Yosys (System: ${yosys_installed_version}, Required: ${required_version})"
    if [[ "${yosys_installed_version}" != "${required_version}" ]]; then
        (
            cd "${BASE_DIR}"
            _execute "Cloning Yosys ${YOSYS_VERSION}..." git clone --depth=1 -b "${YOSYS_VERSION}" --recursive https://github.com/YosysHQ/yosys
            cd yosys
            _execute "Building Yosys..." make -j "${NUM_THREADS}" PREFIX="${yosys_prefix}" ABC_ARCHFLAGS=-Wno-register
            _execute "Installing Yosys..." make install PREFIX="${yosys_prefix}"
        )
        INSTALL_SUMMARY+=("Yosys: system=${yosys_installed_version}, required=${required_version}, path=${yosys_prefix}, status=installed")
    else
        INSTALL_SUMMARY+=("Yosys: system=${yosys_installed_version}, required=${required_version}, path=${yosys_prefix}, status=skipped")
    fi
}

# ------------------------------------------------------------------------------
# Eqy
# ------------------------------------------------------------------------------
_install_eqy() {
    local eqy_prefix=${PREFIX:-"/usr/local"}
    local eqy_bin=${eqy_prefix}/bin/eqy
    local eqy_installed_version="none"
    if [[ -f "${eqy_bin}" ]]; then
        eqy_installed_version=$("${eqy_bin}" --version | awk '{print $2}')
    elif _command_exists "eqy" && [[ -z "${PREFIX}" ]]; then
        eqy_installed_version=$(eqy --version | awk '{print $2}')
    fi

    local required_version="${YOSYS_VERSION}"
    log "Checking Eqy (System: ${eqy_installed_version}, Required: ${required_version})"
    if [[ "${eqy_installed_version}" != "${required_version}" ]]; then
        (
            cd "${BASE_DIR}"
            _execute "Cloning Eqy ${YOSYS_VERSION}..." git clone --depth=1 -b "${YOSYS_VERSION}" https://github.com/YosysHQ/eqy
            cd eqy
            export PATH="${PREFIX:-/usr/local}/bin:${PATH}"
            _execute "Building Eqy..." make -j "${NUM_THREADS}" PREFIX="${eqy_prefix}"
            _execute "Installing Eqy..." make install PREFIX="${eqy_prefix}"
        )
        INSTALL_SUMMARY+=("Eqy: system=${eqy_installed_version}, required=${required_version}, path=${eqy_prefix}, status=installed")
    else
        INSTALL_SUMMARY+=("Eqy: system=${eqy_installed_version}, required=${required_version}, path=${eqy_prefix}, status=skipped")
    fi
}

# ------------------------------------------------------------------------------
# Sby
# ------------------------------------------------------------------------------
_install_sby() {
    local sby_prefix=${PREFIX:-"/usr/local"}
    local sby_bin=${sby_prefix}/bin/sby
    local sby_installed_version="none"
    if [[ -f "${sby_bin}" ]]; then
        sby_installed_version=$("${sby_bin}" --version | awk '{print $2}')
    elif _command_exists "sby" && [[ -z "${PREFIX}" ]]; then
        sby_installed_version=$(sby --version | awk '{print $2}')
    fi

    local required_version="${YOSYS_VERSION}"
    log "Checking Sby (System: ${sby_installed_version}, Required: ${required_version})"
    if [[ "${sby_installed_version}" != "${required_version}" ]]; then
        (
            cd "${BASE_DIR}"
            _execute "Cloning Sby ${YOSYS_VERSION}..." git clone --depth=1 -b "${YOSYS_VERSION}" --recursive https://github.com/YosysHQ/sby
            cd sby
            export PATH="${PREFIX:-/usr/local}/bin:${PATH}"
            _execute "Installing Sby..." make -j "${NUM_THREADS}" PREFIX="${sby_prefix}" install
        )
        INSTALL_SUMMARY+=("Sby: system=${sby_installed_version}, required=${required_version}, path=${sby_prefix}, status=installed")
    else
        INSTALL_SUMMARY+=("Sby: system=${sby_installed_version}, required=${required_version}, path=${sby_prefix}, status=skipped")
    fi
}

_install_equivalence_deps() {
    log "Install equivalence dependencies (-eqy)..."
    _install_yosys
    _install_eqy
    _install_sby
}

# logging, error handling, and other utility tasks.
# ------------------------------------------------------------------------------
# Execute a command, hiding output unless in verbose mode or an error occurs.
# Usage: _execute "Cloning Yosys..." git clone ...
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
# Each dependency will have its own dedicated function for installation and
# version management. This modular approach makes the script easier to
# maintain and extend.
# ------------------------------------------------------------------------------
# Bazel
# ------------------------------------------------------------------------------
_install_bazel() {
    local bazel_prefix=${PREFIX:-"/usr/local"}
    log "Checking Bazel (via bazelisk)"
    if _command_exists "bazelisk"; then
        log "bazelisk already installed, skipping."
        INSTALL_SUMMARY+=("Bazel: system=found, required=any, status=skipped")
        return
    fi
    if [[ "$OSTYPE" == "darwin"* ]]; then
        _execute "Installing bazelisk via Homebrew..." brew install bazelisk
    else
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
        (
            cd "${BASE_DIR}"
            _execute "Downloading bazelisk v${BAZELISK_VERSION}..." curl -Lo bazelisk \
                "https://github.com/bazelbuild/bazelisk/releases/download/v${BAZELISK_VERSION}/bazelisk-linux-${bazelisk_arch}"
            _verify_checksum "${bazelisk_checksum}" "bazelisk" || error "Bazelisk checksum failed."
            chmod +x bazelisk
            _execute "Installing bazelisk..." mv bazelisk "${bazel_prefix}/bin/bazelisk"
        )
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
            _execute "Installing bazel required libraries..." \
                apt-get -y install --no-install-recommends \
                libc6-dev "${libxml2_pkg}" libtinfo6 zlib1g libstdc++6
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
        fi
        if [[ "${NO_GUI}" != "yes" ]]; then
            # Install xcb libraries needed for GUI support with Bazel builds
            if _command_exists "apt-get"; then
                _execute "Installing xcb libraries for GUI support..." \
                    apt-get -y install --no-install-recommends \
                    libxcb1-dev libxcb-util-dev libxcb-icccm4-dev libxcb-image0-dev \
                    libxcb-keysyms1-dev libxcb-randr0-dev libxcb-render-util0-dev \
                    libxcb-xinerama0-dev libxcb-xkb-dev \
                    libx11-xcb1 libx11-6 libsm6 libice6 \
                    libxcb-cursor0 libxcb-shape0 libxcb-sync1 libxcb-xfixes0 \
                    libdbus-1-3 libfontconfig1 libxkbcommon0 libxkbcommon-x11-0
            elif _command_exists "yum"; then
                _execute "Installing xcb libraries for GUI support..." \
                    yum install -y \
                    libxcb-devel xcb-util-devel xcb-util-image-devel \
                    xcb-util-keysyms-devel xcb-util-renderutil-devel xcb-util-wm-devel \
                    libX11-xcb libX11 libSM libICE \
                    xcb-util-cursor libxcb \
                    dbus-libs fontconfig \
                    libxkbcommon libxkbcommon-x11
            fi
        fi
    fi
    INSTALL_SUMMARY+=("Bazel: system=none, required=latest, status=installed")
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
        (
            cd "${BASE_DIR}"
            _execute "Downloading buildifier v${BUILDIFIER_VERSION}..." curl -Lo buildifier \
                "https://github.com/bazelbuild/buildtools/releases/download/v${BUILDIFIER_VERSION}/buildifier-linux-${buildifier_arch}"
            _verify_checksum "${buildifier_checksum}" "buildifier" || error "Buildifier checksum failed."
            chmod +x buildifier
            _execute "Installing buildifier..." mv buildifier "${bazel_prefix}/bin/buildifier"
        )
    fi
    INSTALL_SUMMARY+=("buildifier: system=none, required=latest, status=installed")
}

# ==============================================================================
# Platform-Specific Logic
# ==============================================================================
# This section will contain functions for installing platform-specific packages
# using the system's package manager (e.g., apt, yum, brew).

# ... platform-specific functions will be added here ...

# ==============================================================================
# Main Execution
# ==============================================================================
# The main part of the script will parse command-line arguments, detect the
# platform, and call the appropriate functions to install the dependencies.

# ... main execution logic will be added here ...
# ------------------------------------------------------------------------------
# Ubuntu
# ------------------------------------------------------------------------------
_install_ubuntu_packages() {
    log "Install ubuntu base packages using apt (-base or -all)"
    export DEBIAN_FRONTEND="noninteractive"
    _execute "Updating package lists..." apt-get -y update
    _execute "Installing base packages..." apt-get -y install --no-install-recommends \
        automake autotools-dev binutils bison build-essential ccache clang cmake \
        debhelper devscripts flex g++ gcc git groff lcov libbz2-dev libffi-dev libfl-dev \
        libgomp1 libomp-dev libpcre2-dev libreadline-dev ninja-build pandoc \
        pkg-config python3-dev qt5-image-formats-plugins tcl tcl-dev \
        tcllib unzip wget libyaml-cpp-dev zlib1g-dev tzdata

    local packages=()
    if _version_compare "$1" -ge "25.04"; then
        packages+=("libtcl8.6")
    else
        packages+=("libtcl")
    fi
    if _version_compare "$1" -ge "26.04"; then
        packages+=("libpython3.14")
    elif _version_compare "$1" -ge "25.04"; then
        packages+=("libpython3.13")
    elif _version_compare "$1" -ge "24.04"; then
        packages+=("libpython3.12")
    elif _version_compare "$1" -ge "22.10"; then
        packages+=("libpython3.11")
    else
        packages+=("libpython3.8")
    fi
    if _version_compare "$1" -ge "22.04"; then
        packages+=("qt5-qmake" "qtbase5-dev" "qtbase5-dev-tools" "libqt5charts5-dev" "qtchooser")
    else
        packages+=("libqt5charts5-dev" "qt5-default")
    fi
    _execute "Installing version-specific packages..." apt-get install -y --no-install-recommends "${packages[@]}"
    _execute "Cleaning up packages..." apt-get autoclean -y
    _execute "Removing unused packages..." apt-get autoremove -y
}

# ------------------------------------------------------------------------------
# RHEL
# ------------------------------------------------------------------------------
_install_rhel_packages() {
    log "Install rhel base packages using yum (-base or -all)"
    local rhel_version=$1
    _execute "Updating packages..." yum -y update
    _execute "Installing EPEL release..." yum -y install "https://dl.fedoraproject.org/pub/epel/epel-release-latest-${rhel_version}.noarch.rpm"
    _execute "Installing base packages..." yum -y install \
        autoconf automake clang clang-devel cmake gcc gcc-c++ gdb git glibc-devel \
        bzip2-devel libffi-devel libtool llvm llvm-devel llvm-libs make \
        pcre2-devel pkg-config pkgconf pkgconf-m4 pkgconf-pkg-config python3 \
        python3-devel python3-pip qt5-qtbase-devel qt5-qtcharts-devel \
        qt5-qtimageformats readline-devel tcl-devel \
        tcl-thread-devel tcllib wget yaml-cpp-devel \
        zlib-devel tzdata redhat-rpm-config rpm-build

    if [[ "${rhel_version}" == "8" ]]; then
        local python_version="3.12"
        _execute "Installing Python ${python_version}..." yum install -y gcc-toolset-13 "python${python_version}" "python${python_version}-devel" "python${python_version}-pip"
        _execute "Setting Python alternatives..." update-alternatives --install /usr/bin/unversioned-python python "$(command -v "python${python_version}")" 50
        _execute "Setting Python3 alternatives..." update-alternatives --install /usr/bin/python3 python3 "$(command -v "python${python_version}")" 50
    fi
    if [[ "${rhel_version}" == "9" ]]; then
        _execute "Installing additional packages for RHEL 9..." yum install -y \
            https://mirror.stream.centos.org/9-stream/AppStream/x86_64/os/Packages/flex-2.6.4-9.el9.x86_64.rpm \
            https://rpmfind.net/linux/centos-stream/9-stream/AppStream/x86_64/os/Packages/tcl-devel-8.6.10-7.el9.x86_64.rpm
    fi

    local arch=amd64
    local pandoc_version="3.1.11.1"
    _execute "Downloading pandoc..." wget $OPT_NOCERT "https://github.com/jgm/pandoc/releases/download/${pandoc_version}/pandoc-${pandoc_version}-linux-${arch}.tar.gz"
    _execute "Installing pandoc..." tar xvzf "pandoc-${pandoc_version}-linux-${arch}.tar.gz" --strip-components 1 -C /usr/local/
    rm -rf "pandoc-${pandoc_version}-linux-${arch}.tar.gz"
    _execute "Cleaning up yum cache..." yum clean -y all
    rm -rf /var/lib/apt/lists/*
}

# ------------------------------------------------------------------------------
# openSUSE
# ------------------------------------------------------------------------------
_install_opensuse_packages() {
    log "Install openSUSE base packages using zypper (-base or -all)"
    _execute "Refreshing repositories..." zypper refresh
    _execute "Updating packages..." zypper -n update
    _execute "Installing development pattern..." zypper -n install -t pattern devel_basis
    _execute "Installing base packages..." zypper -n install \
        binutils clang cmake gcc gcc11-c++ git groff gzip lcov libbz2-devel libffi-devel \
        libgomp1 libomp11-devel libpython3_6m1_0 libqt5-creator libqt5-qtbase \
        libqt5-qtstyleplugins libstdc++6-devel-gcc8 llvm pandoc \
        pcre2-devel pkg-config python3-devel python3-pip readline-devel tcl \
        tcl-devel tcllib wget yaml-cpp-devel zlib-devel

    _execute "Setting gcc alternatives..." update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 50
    _execute "Setting g++ alternatives..." update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 50
    _execute "Cleaning up zypper cache..." zypper -n clean --all
    _execute "Removing unneeded packages..." bash -c "zypper -n packages --unneeded | awk -F'|' 'NR > 4 {print \$3}' | grep -v Name | xargs -r zypper -n remove --clean-deps"
}

# ------------------------------------------------------------------------------
# Darwin (macOS)
# ------------------------------------------------------------------------------
_install_darwin_packages() {
    if ! _command_exists "brew"; then
        error "Homebrew is not found. Please install Homebrew before continuing."
    fi
    if ! xcode-select -p &> /dev/null; then
        cat <<EOF
Xcode command line tools not installed.
Run the following command to install them:
  xcode-select --install
Then, rerun this script.
EOF
        exit 1
    fi
    log "Install darwin base packages using homebrew (-base or -all)"
    _execute "Installing Homebrew packages..." brew install bison boost bzip2 cmake eigen flex fmt groff googletest icu4c libomp or-tools pandoc pkg-config qt@5 python readline spdlog tcl-tk@8 zlib swig yaml-cpp
    # _execute "Installing pipx..." brew install pipx
    _execute "Installing Python click..." pip install click
    _execute "Linking libomp..." brew link --force libomp
    _execute "Installing lemon-graph..." brew install The-OpenROAD-Project/lemon-graph/lemon-graph
}

# ------------------------------------------------------------------------------
# Debian
# ------------------------------------------------------------------------------
_install_debian_packages() {
    log "Install debian base packages using apt (-base or -all)"
    local debian_version=$1
    export DEBIAN_FRONTEND="noninteractive"
    _execute "Updating package lists..." apt-get -y update
    local tcl_ver=""
    if [[ "${debian_version}" == "rodete" ]] || _version_compare "${debian_version}" -ge "13"; then
        tcl_ver="8.6"
    fi
    _execute "Installing base packages..." apt-get -y install --no-install-recommends \
        automake autotools-dev binutils bison build-essential clang cmake debhelper \
        devscripts flex g++ gcc git groff lcov libbz2-dev libffi-dev libfl-dev libgomp1 \
        libomp-dev libpcre2-dev libreadline-dev "libtcl${tcl_ver}" ninja-build \
        pandoc pkg-config python3-dev qt5-image-formats-plugins tcl-dev \
        tcllib unzip wget libyaml-cpp-dev zlib1g-dev tzdata

    if [[ "${debian_version}" == "10" ]]; then
        _execute "Installing Debian 10 specific packages..." apt-get install -y --no-install-recommends libpython3.7 libqt5charts5-dev qt5-default
    else
        local python_ver="3.8"
        if [[ "${debian_version}" == "rodete" ]]; then
            python_ver="3.12"
        elif _version_compare "${debian_version}" -ge "13"; then
            python_ver="3.13"
        fi
        _execute "Installing Debian specific packages..." apt-get install -y --no-install-recommends "libpython${python_ver}" libqt5charts5-dev qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools
    fi
    _execute "Cleaning up packages..." apt-get autoclean -y
    _execute "Removing unused packages..." apt-get autoremove -y
}

# ------------------------------------------------------------------------------
# CI Dependencies
# ------------------------------------------------------------------------------
_install_ci_packages() {
    log "Install CI dependencies (-ci)"
    _execute "Updating package lists..." apt-get -y update
    _execute "Installing CI packages..." apt-get -y install --no-install-recommends \
        apt-transport-https ca-certificates curl gnupg jq lsb-release parallel \
        python3 python3-pandas python3-pip software-properties-common \
        time unzip zip

    _execute "Downloading bazelisk..." curl -Lo bazelisk https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64
    chmod +x bazelisk
    _execute "Installing bazelisk..." mv bazelisk /usr/local/bin/bazelisk

    if _command_exists "docker"; then
        log "Docker is already installed, skipping reinstallation."
        return 0
    fi

    _execute "Creating Docker keyring directory..." install -m 0755 -d /etc/apt/keyrings
    _execute "Downloading Docker GPG key..." curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
    chmod a+r /etc/apt/keyrings/docker.asc
    _execute "Adding Docker repository..." bash -c 'echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
        $(. /etc/os-release && echo \"${VERSION_CODENAME}\") stable" | tee /etc/apt/sources.list.d/docker.list > /dev/null'
    _execute "Updating package lists for Docker..." apt-get -y update
    _execute "Installing Docker..." apt-get -y install --no-install-recommends docker-ce docker-ce-cli containerd.io docker-buildx-plugin

    if _version_compare "${1}" -lt "24.04"; then
        _execute "Downloading LLVM install script..." wget $OPT_NOCERT https://apt.llvm.org/llvm.sh
        chmod +x llvm.sh
        _execute "Installing LLVM 16..." ./llvm.sh 16 all
    fi
}
_help() {
    cat <<EOF

Usage: $0 [OPTIONS]

Options:
  -base                       Install base dependencies using package managers. Requires privileged access.
  -eqy                        Install equivalence dependencies (yosys, eqy, sby).
  -bazel                      Download and install bazel (via bazelisk).
  -bazel-dev                  Download and install bazel developer tools (buildifier, etc.).
  -no-gui                     Skip GUI-only dependencies (e.g. xcb libraries) when used with -bazel.
  -prefix=DIR                 Install dependencies in a user-specified directory.
  -local                      Install dependencies in \${HOME}/.local.
  -ci                         Install dependencies required for CI.
  -nocert                     Disable certificate checks for downloads.
  -constant-build-dir         Use a constant build directory instead of a random one.
  -threads=<N>                Limit the number of compiling threads.
  -yosys-ver=<VERSION>        Specify a custom Yosys version. Used for ORFS.
  -verbose                    Show all output from build commands.
  -h, -help                   Show this help message.

C++ build dependencies (boost, or-tools, swig, ...) are not installed by
this script; they come from the bazel module graph. For a plain CMake
build, materialize them with: bazelisk run //:cmake (see docs/user/Build.md).

EOF
    exit "${1:-1}"
}

main() {
    local option="none"
    while [ "$#" -gt 0 ]; do
        case "${1}" in
            -h|-help) _help 0 ;;
            -all|-common)
                error "${1} was removed: C++ dependencies come from the bazel module graph. Use 'bazelisk run //:cmake' for plain CMake builds (see docs/user/Build.md)."
                ;;
            -base) option="base" ;;
            -eqy) EQUIVALENCE_DEPS="yes" ;;
            -bazel) INSTALL_BAZEL="yes" ;;
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
                OPT_NOCERT="--no-check-certificate"
                export GIT_SSL_NO_VERIFY=true
                ;;
            -threads=*) NUM_THREADS="${1#*=}" ;;
            -yosys-ver=*) YOSYS_VERSION="${1#*=}" ;;
            *)
                echo "Unknown option: ${1}" >&2
                _help
                ;;
        esac
        shift 1
    done

    if [[ "${option}" == "none" && "${INSTALL_BAZEL}" == "no" \
        && "${INSTALL_BAZEL_DEV}" == "no" && "${EQUIVALENCE_DEPS}" == "no" ]]; then
        error "You must use one of: -base, -eqy, -bazel, or -bazel-dev."
    fi

    # -bazel-dev implies -bazel (you need bazelisk to use buildifier)
    if [[ "${INSTALL_BAZEL}" == "yes" || "${INSTALL_BAZEL_DEV}" == "yes" ]]; then
        _install_bazel
    fi

    if [[ "${INSTALL_BAZEL_DEV}" == "yes" ]]; then
        _install_bazel_dev
    fi

    if [[ "${option}" == "none" && "${EQUIVALENCE_DEPS}" == "no" ]]; then
        _print_summary
        rm -rf "${BASE_DIR}"
        return
    fi

    local platform
    platform="$(uname -s)"
    case "${platform}" in
        "Linux")
            if [[ ! -f /etc/os-release ]]; then
                error "Unidentified OS, could not find /etc/os-release."
            fi
            local os
            os=$(awk -F= '/^NAME/{print $2}' /etc/os-release | sed 's/"//g')
            ;;
        "Darwin")
            if [[ $(id -u) == 0 ]]; then
                error "Cannot install on macOS with root or sudo."
            fi
            os="Darwin"
            ;;
        *)
            error "${platform} is not supported. We only officially support Linux at the moment."
            ;;
    esac

    case "${os}" in
        "Ubuntu")
            local ubuntu_version
            ubuntu_version=$(awk -F= '/^VERSION_ID/{print $2}' /etc/os-release | sed 's/"//g')
            if [[ "${option}" == "base" ]]; then
                _install_ubuntu_packages "${ubuntu_version}"
            fi
            if [[ "${CI}" == "yes" ]]; then
                _install_ci_packages "${ubuntu_version}"
            fi
            ;;
        "Red Hat Enterprise Linux" | "Rocky Linux" | "AlmaLinux")
            local rhel_version
            if [[ "${os}" == "Red Hat Enterprise Linux" ]]; then
                rhel_version=$(rpm -q --queryformat '%{VERSION}' redhat-release | cut -d. -f1)
            elif [[ "${os}" == "Rocky Linux" ]]; then
                rhel_version=$(rpm -q --queryformat '%{VERSION}' rocky-release | cut -d. -f1)
            elif [[ "${os}" == "AlmaLinux" ]]; then
                rhel_version=$(rpm -q --queryformat '%{VERSION}' almalinux-release | cut -d. -f1)
            fi
            if [[ "${rhel_version}" != "8" && "${rhel_version}" != "9" ]]; then
                error "Unsupported ${rhel_version} version. Versions '8' and '9' are supported."
            fi
            if [[ "${option}" == "base" ]]; then
                _install_rhel_packages "${rhel_version}"
            fi
            ;;
        "Darwin")
            if [[ "${option}" == "base" ]]; then
                _install_darwin_packages
                cat <<EOF

To install or run OpenROAD, update your path with:
    export PATH="\$(brew --prefix bison)/bin:\$(brew --prefix flex)/bin:\$(brew --prefix tcl-tk@8)/bin:\${PATH}"
    export CMAKE_PREFIX_PATH=\$(brew --prefix or-tools)
EOF
            fi
            ;;
        "openSUSE Leap")
            if [[ "${option}" == "base" ]]; then
                _install_opensuse_packages
            fi
            cat <<EOF
To enable GCC-11 you need to run:
        update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 50
        update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 50
EOF
            ;;
        "Debian GNU/Linux" | "Debian GNU/Linux rodete")
            local debian_version
            debian_version=$(awk -F= '/^VERSION_ID/{print $2}' /etc/os-release | sed 's/"//g')
            if [[ -z "${debian_version}" ]]; then
                debian_version=$(awk -F= '/^VERSION_CODENAME/{print $2}' /etc/os-release | sed 's/"//g')
            fi
            if [[ "${option}" == "base" ]]; then
                _install_debian_packages "${debian_version}"
            fi
            ;;
        *)
            error "Unsupported system: ${os}"
            ;;
    esac

    if [[ "${EQUIVALENCE_DEPS}" == "yes" ]]; then
        rm -rf "${BASE_DIR}"
        mkdir -p "${BASE_DIR}"
        if [[ -n "${PREFIX}" ]]; then
            mkdir -p "${PREFIX}"
        fi
        _install_equivalence_deps
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
        # summary_line is like "Flex: system=2.6.4, required=2.6.4, path=/usr/local, status=skipped"
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
