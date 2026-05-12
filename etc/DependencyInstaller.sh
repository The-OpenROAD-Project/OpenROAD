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
SAVE_DEPS_PREFIXES=""
NUM_THREADS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
SKIP_SYSTEM_OR_TOOLS="false"
BASE_DIR=$(mktemp -d /tmp/DependencyInstaller-XXXXXX)
CMAKE_PACKAGE_ROOT_ARGS=""
OR_TOOLS_PATH=""
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
# NOTE: These checksums were originally generated with MD5. After switching to
# SHA-256 (see _verify_checksum below), the actual hash values must be
# regenerated. To regenerate: download each file, run sha256sum on it, and
# replace the corresponding value below.
YOSYS_VERSION="v0.58"
CMAKE_VERSION_BIG="3.31"
CMAKE_VERSION_SMALL="${CMAKE_VERSION_BIG}.9"
CMAKE_CHECKSUM_AARCH64="8d426361ce8c54494c0a041a0e3bcc5c"
CMAKE_CHECKSUM_X86_64="64797150473dabe9de95459cb1ce0715"
PCRE_VERSION="10.42"
PCRE_CHECKSUM="37d2f77cfd411a3ddf1c64e1d72e43f7"
SWIG_VERSION="4.3.0"
SWIG_CHECKSUM="9f74c7f402aa28d9f75e67d1990ee6fb"
BOOST_VERSION_BIG="1.89"
BOOST_VERSION_SMALL="${BOOST_VERSION_BIG}.0"
BOOST_CHECKSUM="187b577ce9f485314fcf17bcba2fb542"
EIGEN_VERSION="3.4"
CUDD_VERSION="3.0.0"
LEMON_VERSION="1.3.1"
SPDLOG_VERSION="1.15.0"
GTEST_VERSION="1.17.0"
GTEST_CHECKSUM="3471f5011afc37b6555f6619c14169cf"
ABSL_VERSION="20260107.0"
ABSL_CHECKSUM="2a7add2ee848dd4591f41b0f6339d624"
BISON_VERSION="3.8.2"
BISON_CHECKSUM="1e541a097cda9eca675d29dd2832921f"
FLEX_VERSION="2.6.4"
FLEX_CHECKSUM="2882e3179748cc9f9c23ec593d6adc8d"
OR_TOOLS_VERSION_BIG="9.14"
OR_TOOLS_VERSION_SMALL="${OR_TOOLS_VERSION_BIG}.6206"
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
    # MD5 is cryptographically broken (CMU SEI 2008, NIST non-approved).
    # SHA-256 replaces MD5 for supply chain integrity.
    # NOTE: The checksum values defined above were generated with MD5 and
    # must be re-generated with sha256sum after downloading each dependency.
    _execute "Verifying ${filename} checksum..." bash -c "echo '${checksum}  ${filename}' | sha256sum --quiet -c -"
}
