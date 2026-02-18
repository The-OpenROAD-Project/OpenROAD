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
NUM_THREADS=$(nproc)
SKIP_SYSTEM_OR_TOOLS="false"
BASE_DIR=$(mktemp -d /tmp/DependencyInstaller-XXXXXX)
CMAKE_PACKAGE_ROOT_ARGS=""
OR_TOOLS_PATH=""
INSTALL_SUMMARY=()
VERBOSE_MODE="no"

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
NINJA_VERSION="1.10.2"
NINJA_CHECKSUM="817e12e06e2463aeb5cb4e1d19ced606"
OR_TOOLS_VERSION_BIG="9.14"
OR_TOOLS_VERSION_SMALL="${OR_TOOLS_VERSION_BIG}.6206"
EQUIVALENCE_DEPS="no"
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
_install_yosys() {
    local yosys_prefix=${PREFIX:-"/usr/local"}
    local yosys_installed_version="none"
    if _command_exists "yosys"; then
        yosys_installed_version=$(yosys -V | awk '{print $2}')
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
        INSTALL_SUMMARY+=("Yosys: system=${yosys_installed_version}, required=${required_version}, status=installed")
    else
        INSTALL_SUMMARY+=("Yosys: system=${yosys_installed_version}, required=${required_version}, status=skipped")
    fi
}

# ------------------------------------------------------------------------------
# Eqy
# ------------------------------------------------------------------------------
_install_eqy() {
    local eqy_prefix=${PREFIX:-"/usr/local"}
    local eqy_installed_version="none"
    if _command_exists "eqy"; then
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
        INSTALL_SUMMARY+=("Eqy: system=${eqy_installed_version}, required=${required_version}, status=installed")
    else
        INSTALL_SUMMARY+=("Eqy: system=${eqy_installed_version}, required=${required_version}, status=skipped")
    fi
}

# ------------------------------------------------------------------------------
# Sby
# ------------------------------------------------------------------------------
_install_sby() {
    local sby_prefix=${PREFIX:-"/usr/local"}
    local sby_installed_version="none"
    if _command_exists "sby"; then
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
        INSTALL_SUMMARY+=("Sby: system=${sby_installed_version}, required=${required_version}, status=installed")
    else
        INSTALL_SUMMARY+=("Sby: system=${sby_installed_version}, required=${required_version}, status=skipped")
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

    echo -n "${BLUE}${BOLD}[INFO]${NC} ${description}..."
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

# CMake
# ------------------------------------------------------------------------------
_install_cmake() {
    local cmake_prefix=${PREFIX:-"/usr/local"}
    local cmake_bin=${cmake_prefix}/bin/cmake
    local cmake_installed_version="none"
    if [[ -f ${cmake_bin} ]]; then
        cmake_installed_version=$(${cmake_bin} --version | head -n1 | awk '{print $3}')
    fi

    log "Checking CMake (System: ${cmake_installed_version}, Required: ${CMAKE_VERSION_SMALL})"
    if [[ "${cmake_installed_version}" != "${CMAKE_VERSION_SMALL}" ]]; then
        (
            cd "${BASE_DIR}"
            local arch
            arch=$(uname -m)
            local cmake_checksum=""
            if [[ "${arch}" == "aarch64" ]]; then
                cmake_checksum=${CMAKE_CHECKSUM_AARCH64}
            else
                cmake_checksum=${CMAKE_CHECKSUM_X86_64}
            fi
            _execute "Downloading CMake..." wget "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION_SMALL}/cmake-${CMAKE_VERSION_SMALL}-linux-${arch}.sh"
            _verify_checksum "${cmake_checksum}" "cmake-${CMAKE_VERSION_SMALL}-linux-${arch}.sh" || error "CMake checksum failed."
            chmod +x "cmake-${CMAKE_VERSION_SMALL}-linux-${arch}.sh"
            _execute "Installing CMake..." "./cmake-${CMAKE_VERSION_SMALL}-linux-${arch}.sh" --skip-license --prefix="${cmake_prefix}"
        )
        INSTALL_SUMMARY+=("CMake: system=${cmake_installed_version}, required=${CMAKE_VERSION_SMALL}, status=installed")
    else
        INSTALL_SUMMARY+=("CMake: system=${cmake_installed_version}, required=${CMAKE_VERSION_SMALL}, status=skipped")
    fi
}

# ------------------------------------------------------------------------------
# Bison
# ------------------------------------------------------------------------------
_install_bison() {
    local bison_prefix=${PREFIX:-"/usr"}
    local bison_installed_version="none"
    if _command_exists "bison"; then
        bison_installed_version=$(bison --version | awk 'NR==1 {print $NF}')
    fi

    log "Checking Bison (System: ${bison_installed_version}, Required: ${BISON_VERSION})"
    if [[ "${bison_installed_version}" != "${BISON_VERSION}" ]]; then
        (
            cd "${BASE_DIR}"
            local mirrors=(
                "https://ftp.gnu.org/gnu/bison"
                "https://ftpmirror.gnu.org/bison"
                "https://mirrors.kernel.org/gnu/bison"
                "https://mirrors.dotsrc.org/gnu/bison"
            )
            local success=0
            for mirror in "${mirrors[@]}"; do
                local url="${mirror}/bison-${BISON_VERSION}.tar.gz"
                log "Trying to download bison from: $url"
                if wget "$url"; then
                    success=1
                    break
                else
                    warn "Failed to download from $mirror"
                fi
            done
            if [[ ${success} -ne 1 ]]; then
                error "Could not download bison-${BISON_VERSION}.tar.gz from any mirror."
            fi
            _verify_checksum "${BISON_CHECKSUM}" "bison-${BISON_VERSION}.tar.gz" || error "Bison checksum failed."
            _execute "Extracting Bison..." tar xf "bison-${BISON_VERSION}.tar.gz"
            cd "bison-${BISON_VERSION}"
            _execute "Configuring Bison..." ./configure --prefix="${bison_prefix}"
            _execute "Building and installing Bison..." make -j "${NUM_THREADS}" install
        )
        INSTALL_SUMMARY+=("Bison: system=${bison_installed_version}, required=${BISON_VERSION}, status=installed")
    else
        INSTALL_SUMMARY+=("Bison: system=${bison_installed_version}, required=${BISON_VERSION}, status=skipped")
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D bison_ROOT=$(realpath "${bison_prefix}") "
}

# ------------------------------------------------------------------------------
# Flex
# ------------------------------------------------------------------------------
_install_flex() {
    local flex_prefix=${PREFIX:-"/usr/local"}
    local flex_installed_version="none"
    if _command_exists "flex"; then
        flex_installed_version=$(flex --version | awk '{print $2}')
    fi

    log "Checking Flex (System: ${flex_installed_version}, Required: ${FLEX_VERSION})"
    if [[ "${flex_installed_version}" != "${FLEX_VERSION}" ]]; then
        (
            cd "${BASE_DIR}"
            _execute "Downloading Flex..." wget https://github.com/westes/flex/releases/download/v${FLEX_VERSION}/flex-${FLEX_VERSION}.tar.gz
            _verify_checksum "${FLEX_CHECKSUM}" "flex-${FLEX_VERSION}.tar.gz" || error "Flex checksum failed."
            _execute "Extracting Flex..." tar xf "flex-${FLEX_VERSION}.tar.gz"
            cd "flex-${FLEX_VERSION}"
            _execute "Configuring Flex..." ./configure --prefix="${flex_prefix}"
            _execute "Building Flex..." make -j "${NUM_THREADS}"
            _execute "Installing Flex..." make -j "${NUM_THREADS}" install
        )
        INSTALL_SUMMARY+=("Flex: system=${flex_installed_version}, required=${FLEX_VERSION}, status=installed")
    else
        INSTALL_SUMMARY+=("Flex: system=${flex_installed_version}, required=${FLEX_VERSION}, status=skipped")
    fi
}

# ------------------------------------------------------------------------------
# SWIG
# ------------------------------------------------------------------------------
_install_swig() {
    local swig_prefix=${PREFIX:-"/usr/local"}
    local swig_bin=${swig_prefix}/bin/swig
    local swig_installed_version="none"
    if [[ -f ${swig_bin} ]]; then
        swig_installed_version=$(${swig_bin} -version | grep "SWIG Version" | awk '{print $3}')
    fi

    log "Checking SWIG (System: ${swig_installed_version}, Required: ${SWIG_VERSION})"
    if [[ "${swig_installed_version}" != "${SWIG_VERSION}" ]]; then
        (
            cd "${BASE_DIR}"
            local tar_name="v${SWIG_VERSION}.tar.gz"
            _execute "Downloading SWIG..." wget "https://github.com/swig/swig/archive/${tar_name}"
            _verify_checksum "${SWIG_CHECKSUM}" "${tar_name}" || error "SWIG checksum failed."
            _execute "Extracting SWIG..." tar xfz "${tar_name}"
            cd swig-*

            _install_pcre
            _execute "Generating SWIG configure script..." ./autogen.sh
            _execute "Configuring SWIG..." ./configure --prefix="${swig_prefix}"
            _execute "Building SWIG..." make -j "${NUM_THREADS}"
            _execute "Installing SWIG..." make -j "${NUM_THREADS}" install
        )
        INSTALL_SUMMARY+=("SWIG: system=${swig_installed_version}, required=${SWIG_VERSION}, status=installed")
    else
        INSTALL_SUMMARY+=("SWIG: system=${swig_installed_version}, required=${SWIG_VERSION}, status=skipped")
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D SWIG_ROOT=$(realpath "${swig_prefix}") "
}

# ------------------------------------------------------------------------------
# PCRE
# ------------------------------------------------------------------------------
_install_pcre() {
    local pcre_prefix=${PREFIX:-"/usr/local"}
    local pcre_config=${pcre_prefix}/bin/pcre2-config
    local pcre_installed_version="none"

    if [[ -f "${pcre_config}" ]]; then
        pcre_installed_version=$(${pcre_config} --version)
    fi

    log "Checking PCRE (System: ${pcre_installed_version}, Required: ${PCRE_VERSION})"
    if [[ "${pcre_installed_version}" == "${PCRE_VERSION}" ]]; then
        INSTALL_SUMMARY+=("PCRE: system=${pcre_installed_version}, required=${PCRE_VERSION}, status=skipped")
        return
    fi

    (
        cd "${BASE_DIR}"
        local pcre_tar_name="pcre2-${PCRE_VERSION}.tar.gz"
        _execute "Downloading PCRE..." wget "https://github.com/PCRE2Project/pcre2/releases/download/pcre2-${PCRE_VERSION}/${pcre_tar_name}"
        _verify_checksum "${PCRE_CHECKSUM}" "${pcre_tar_name}" || error "PCRE checksum failed."
        _execute "Extracting PCRE..." tar xf "${pcre_tar_name}"
        cd "pcre2-${PCRE_VERSION}"
        _execute "Configuring PCRE..." ./configure --prefix="${pcre_prefix}"
        _execute "Building PCRE..." make -j "${NUM_THREADS}"
        _execute "Installing PCRE..." make -j "${NUM_THREADS}" install
    )
    INSTALL_SUMMARY+=("PCRE: system=${pcre_installed_version}, required=${PCRE_VERSION}, status=installed")
}


# ------------------------------------------------------------------------------
# Boost
# ------------------------------------------------------------------------------
_install_boost() {
    local boost_prefix=${PREFIX:-"/usr/local"}
    local boost_installed_version="none"
    if [[ -f "${boost_prefix}/include/boost/version.hpp" ]]; then
        boost_installed_version=$(grep "^#define BOOST_LIB_VERSION" "${boost_prefix}/include/boost/version.hpp" | sed -e 's/.*"\(.*\)"/\1/' -e 's/_/./g')
    fi
    
    local required_version="${BOOST_VERSION_BIG}"
    log "Checking Boost (System: ${boost_installed_version}, Required: ${required_version})"
    if [[ "${boost_installed_version}" != "${required_version}" ]]; then
        (
            cd "${BASE_DIR}"
            local boost_version_underscore=${BOOST_VERSION_SMALL//./_}
            _execute "Downloading Boost..." wget "https://archives.boost.io/release/${BOOST_VERSION_SMALL}/source/boost_${boost_version_underscore}.tar.gz"
            _verify_checksum "${BOOST_CHECKSUM}" "boost_${boost_version_underscore}.tar.gz" || error "Boost checksum failed."
            _execute "Extracting Boost..." tar -xf "boost_${boost_version_underscore}.tar.gz"
            cd "boost_${boost_version_underscore}"
            _execute "Bootstrapping Boost..." ./bootstrap.sh --prefix="${boost_prefix}"
            _execute "Installing Boost..." ./b2 install --with-iostreams --with-test --with-serialization --with-system --with-thread -j "${NUM_THREADS}"
        )
        INSTALL_SUMMARY+=("Boost: system=${boost_installed_version}, required=${required_version}, status=installed")
    else
        INSTALL_SUMMARY+=("Boost: system=${boost_installed_version}, required=${required_version}, status=skipped")
    fi

    local boost_cmake_dir=""
    if [[ -d "${boost_prefix}/lib/cmake/Boost-${BOOST_VERSION_SMALL}" ]]; then
        boost_cmake_dir="${boost_prefix}/lib/cmake/Boost-${BOOST_VERSION_SMALL}"
    elif [[ -d "${boost_prefix}/lib64/cmake/Boost-${BOOST_VERSION_SMALL}" ]]; then
        boost_cmake_dir="${boost_prefix}/lib64/cmake/Boost-${BOOST_VERSION_SMALL}"
    fi

    if [[ -n "${boost_cmake_dir}" ]]; then
        CMAKE_PACKAGE_ROOT_ARGS+=" -D Boost_DIR=$(realpath "${boost_cmake_dir}") "
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D Boost_ROOT=$(realpath "${boost_prefix}") "
}

# ------------------------------------------------------------------------------
# Eigen
# ------------------------------------------------------------------------------
_install_eigen() {
    local eigen_prefix=${PREFIX:-"/usr/local"}
    local eigen_version_file="${eigen_prefix}/include/eigen3/Eigen/src/Core/util/Macros.h"
    local eigen_installed_version="none"

    if [[ -f "${eigen_version_file}" ]]; then
        local world
        world=$(grep "#define EIGEN_WORLD_VERSION" "${eigen_version_file}" | awk '{print $3}')
        local major
        major=$(grep "#define EIGEN_MAJOR_VERSION" "${eigen_version_file}" | awk '{print $3}')
        eigen_installed_version="${world}.${major}"
    fi

    log "Checking Eigen (System: ${eigen_installed_version}, Required: ${EIGEN_VERSION})"
    if [[ "${eigen_installed_version}" != "${EIGEN_VERSION}" ]]; then
        (
            cd "${BASE_DIR}"
            _execute "Cloning Eigen..." git clone --depth=1 -b "${EIGEN_VERSION}" https://gitlab.com/libeigen/eigen.git
            cd eigen
            local cmake_bin=${PREFIX:-/usr/local}/bin/cmake
            _execute "Configuring Eigen..." "${cmake_bin}" -DCMAKE_INSTALL_PREFIX="${eigen_prefix}" -B build .
            _execute "Building and installing Eigen..." "${cmake_bin}" --build build -j "${NUM_THREADS}" --target install
        )
        INSTALL_SUMMARY+=("Eigen: system=${eigen_installed_version}, required=${EIGEN_VERSION}, status=installed")
    else
        INSTALL_SUMMARY+=("Eigen: system=${eigen_installed_version}, required=${EIGEN_VERSION}, status=skipped")
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D Eigen3_ROOT=$(realpath "${eigen_prefix}") "
}

# ------------------------------------------------------------------------------
# CUDD
# ------------------------------------------------------------------------------
_install_cudd() {
    local cudd_prefix=${PREFIX:-"/usr/local"}
    log "Checking CUDD (Required: ${CUDD_VERSION})"
    if [[ ! -f ${cudd_prefix}/include/cudd.h ]]; then
        (
            cd "${BASE_DIR}"
            _execute "Cloning CUDD..." git clone --depth=1 -b "${CUDD_VERSION}" https://github.com/The-OpenROAD-Project/cudd.git
            cd cudd
            _execute "Generating CUDD configure script..." autoreconf
            _execute "Configuring CUDD..." ./configure --prefix="${cudd_prefix}"
            _execute "Building and installing CUDD..." make -j "${NUM_THREADS}" install
        )
        INSTALL_SUMMARY+=("CUDD: system=none, required=${CUDD_VERSION}, status=installed")
    else
        INSTALL_SUMMARY+=("CUDD: system=found, required=${CUDD_VERSION}, status=skipped")
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D cudd_ROOT=$(realpath "${cudd_prefix}") "
}

# ------------------------------------------------------------------------------
# CUSP
# ------------------------------------------------------------------------------
_install_cusp() {
    local cusp_prefix=${PREFIX:-"/usr/local/include"}
    log "Checking CUSP"
    if [[ ! -f ${cusp_prefix}/cusp/version.h ]]; then
        (
            cd "${BASE_DIR}"
            _execute "Cloning CUSP..." git clone --depth=1 -b cuda9 https://github.com/cusplibrary/cusplibrary.git
            cd cusplibrary
            _execute "Installing CUSP..." cp -r ./cusp "${cusp_prefix}"
        )
        INSTALL_SUMMARY+=("CUSP: system=none, required=any, status=installed")
    else
        INSTALL_SUMMARY+=("CUSP: system=found, required=any, status=skipped")
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D cusp_ROOT=$(realpath "${cusp_prefix}") "
}

# ------------------------------------------------------------------------------
# Lemon
# ------------------------------------------------------------------------------
_install_lemon() {
    local lemon_prefix=${PREFIX:-"/usr/local"}
    local lemon_installed_version="none"
    if [[ -f "${lemon_prefix}/include/lemon/config.h" ]]; then
        lemon_installed_version=$(grep "LEMON_VERSION" "${lemon_prefix}/include/lemon/config.h" | awk -F'"' '{print $2}')
    fi

    log "Checking Lemon (System: ${lemon_installed_version}, Required: ${LEMON_VERSION})"
    if [[ "${lemon_installed_version}" != "${LEMON_VERSION}" ]]; then
        (
            cd "${BASE_DIR}"
            _execute "Cloning Lemon..." git clone --depth=1 -b "${LEMON_VERSION}" https://github.com/The-OpenROAD-Project/lemon-graph.git
            cd lemon-graph
            local cmake_bin=${PREFIX:-/usr/local}/bin/cmake
            _execute "Configuring Lemon..." "${cmake_bin}" -DCMAKE_INSTALL_PREFIX="${lemon_prefix}" -B build .
            _execute "Building and installing Lemon..." "${cmake_bin}" --build build -j "${NUM_THREADS}" --target install
        )
        INSTALL_SUMMARY+=("Lemon: system=${lemon_installed_version}, required=${LEMON_VERSION}, status=installed")
    else
        INSTALL_SUMMARY+=("Lemon: system=${lemon_installed_version}, required=${LEMON_VERSION}, status=skipped")
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D LEMON_ROOT=$(realpath "${lemon_prefix}") "
}

# ------------------------------------------------------------------------------
# spdlog
# ------------------------------------------------------------------------------
_install_spdlog() {
    local spdlog_prefix=${PREFIX:-"/usr/local"}
    local spdlog_installed_version="none"
    if [ -d "${spdlog_prefix}/include/spdlog" ]; then
        spdlog_installed_version=$(grep "#define SPDLOG_VER_" "${spdlog_prefix}/include/spdlog/version.h" | sed 's/.*\s//' | tr '\n' '.' | sed 's/\.$//')
    fi
    log "Checking spdlog (System: ${spdlog_installed_version}, Required: ${SPDLOG_VERSION})"
    if [[ "${spdlog_installed_version}" != "${SPDLOG_VERSION}" ]]; then
        (
            cd "${BASE_DIR}"
            _execute "Cloning spdlog..." git clone --depth=1 -b "v${SPDLOG_VERSION}" https://github.com/gabime/spdlog.git
            cd spdlog
            local cmake_bin=${PREFIX:-/usr/local}/bin/cmake
            _execute "Configuring spdlog..." "${cmake_bin}" -DCMAKE_INSTALL_PREFIX="${spdlog_prefix}" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DSPDLOG_BUILD_EXAMPLE=OFF -B build .
            _execute "Building and installing spdlog..." "${cmake_bin}" --build build -j "${NUM_THREADS}" --target install
        )
        INSTALL_SUMMARY+=("spdlog: system=${spdlog_installed_version}, required=${SPDLOG_VERSION}, status=installed")
    else
        INSTALL_SUMMARY+=("spdlog: system=${spdlog_installed_version}, required=${SPDLOG_VERSION}, status=skipped")
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D spdlog_ROOT=$(realpath "${spdlog_prefix}") "
}

# ------------------------------------------------------------------------------
# gtest
# ------------------------------------------------------------------------------
_install_gtest() {
    local gtest_prefix=${PREFIX:-"/usr/local"}
    log "Checking gtest (Required: ${GTEST_VERSION})"
    if [[ ! -d ${gtest_prefix}/include/gtest ]]; then
        (
            cd "${BASE_DIR}"
            _execute "Downloading gtest..." wget "https://github.com/google/googletest/archive/refs/tags/v${GTEST_VERSION}.zip"
            _verify_checksum "${GTEST_CHECKSUM}" "v${GTEST_VERSION}.zip" || error "gtest checksum failed."
            _execute "Extracting gtest..." unzip "v${GTEST_VERSION}.zip"
            cd "googletest-${GTEST_VERSION}"
            local cmake_bin=${PREFIX:-/usr/local}/bin/cmake
            _execute "Configuring gtest..." "${cmake_bin}" -DCMAKE_INSTALL_PREFIX="${gtest_prefix}" -B build .
            _execute "Building and installing gtest..." "${cmake_bin}" --build build --target install
        )
        INSTALL_SUMMARY+=("gtest: system=none, required=${GTEST_VERSION}, status=installed")
    else
        INSTALL_SUMMARY+=("gtest: system=found, required=${GTEST_VERSION}, status=skipped")
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D GTest_ROOT=$(realpath "${gtest_prefix}") "
}

# ------------------------------------------------------------------------------
# Abseil
# ------------------------------------------------------------------------------
_install_abseil() {
    local absl_prefix_install=${PREFIX:-"/usr/local"}
    local absl_prefix_found=""
    local absl_version_file=""

    # Check in default/user-specified prefix first
    local absl_version_file_default="${absl_prefix_install}/lib/cmake/absl/abslConfigVersion.cmake"
    if [[ -f "${absl_version_file_default}" ]]; then
        absl_prefix_found="${absl_prefix_install}"
        absl_version_file="${absl_version_file_default}"
    fi

    # If not found, check in or-tools path
    if [[ -z "${absl_prefix_found}" && -n "${OR_TOOLS_PATH}" ]]; then
        local absl_version_file_or_tools="${OR_TOOLS_PATH}/lib/cmake/absl/abslConfigVersion.cmake"
        if [[ -f "${absl_version_file_or_tools}" ]]; then
            absl_prefix_found="${OR_TOOLS_PATH}"
            absl_version_file="${absl_version_file_or_tools}"
        fi
    fi

    local absl_installed_version="none"
    if [[ -n "${absl_version_file}" ]]; then
        absl_installed_version=$(grep '^set(PACKAGE_VERSION "' "${absl_version_file}" | head -n 1 | awk -F'"' '{print $2}')
    fi

    local required_version="${ABSL_VERSION%.*}"
    log "Checking Abseil (System: ${absl_installed_version}, Required: ${required_version})"
    if [[ "${absl_installed_version}" != "${required_version}" ]]; then
        (
            cd "${BASE_DIR}"
            _execute "Downloading Abseil..." wget "https://github.com/abseil/abseil-cpp/releases/download/${ABSL_VERSION}/abseil-cpp-${ABSL_VERSION}.tar.gz"
            _verify_checksum "${ABSL_CHECKSUM}" "abseil-cpp-${ABSL_VERSION}.tar.gz" || error "Abseil checksum failed."
            _execute "Extracting Abseil..." tar xf "abseil-cpp-${ABSL_VERSION}.tar.gz"
            cd "abseil-cpp-${ABSL_VERSION}"
            local cmake_bin=${PREFIX:-/usr/local}/bin/cmake
            _execute "Configuring Abseil..." "${cmake_bin}" -DCMAKE_INSTALL_PREFIX="${absl_prefix_install}" -DCMAKE_CXX_STANDARD=17 -B build .
            _execute "Building and installing Abseil..." "${cmake_bin}" --build build --target install
        )
        absl_prefix_found="${absl_prefix_install}"
        INSTALL_SUMMARY+=("Abseil: system=${absl_installed_version}, required=${required_version}, status=installed")
    else
        INSTALL_SUMMARY+=("Abseil: system=${absl_installed_version}, required=${required_version}, status=skipped")
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D ABSL_ROOT=$(realpath "${absl_prefix_found}") "
}

# ------------------------------------------------------------------------------
# Ninja
# ------------------------------------------------------------------------------
_install_ninja() {
    local ninja_prefix=${PREFIX:-"/usr/local"}
    local ninja_bin=${ninja_prefix}/bin/ninja
    log "Checking Ninja (Required: ${NINJA_VERSION})"
    if [[ ! -f ${ninja_bin} ]]; then
        (
            cd "${BASE_DIR}"
            _execute "Downloading Ninja..." wget -O ninja-linux.zip "https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-linux.zip"
            _verify_checksum "${NINJA_CHECKSUM}" "ninja-linux.zip" || error "Ninja checksum failed."
            _execute "Installing Ninja..." unzip -o ninja-linux.zip -d "${ninja_prefix}/bin/"
            chmod +x "${ninja_bin}"
        )
        INSTALL_SUMMARY+=("Ninja: system=none, required=${NINJA_VERSION}, status=installed")
    else
        INSTALL_SUMMARY+=("Ninja: system=found, required=${NINJA_VERSION}, status=skipped")
    fi
}

_install_or_tools() {
    local os=$1
    local os_version=$2
    local arch=$3
    local skip_system_or_tools=$4

    # Create a temporary directory for downloads
    local build_dir
    build_dir=$(mktemp -d)

    local or_tools_installed_version="none"
    # Check if a system-installed version of OR-Tools is present
    log "Checking or-tools (System: ${or_tools_installed_version}, Required: ${OR_TOOLS_VERSION_SMALL})"
    if [[ "${skip_system_or_tools}" == "false" ]]; then
        # Disable exit on error for 'find' command, as it might return non zero
        set +euo pipefail
        local existing_libs
        existing_libs=$(find /usr/local /usr /opt -type f -name "libortools.so.*" 2>/dev/null)
        # Bring back exit on error
        set -euo pipefail
        if [[ -n "${existing_libs}" ]]; then
            local first_lib="${existing_libs%%$'\n'*}"
            or_tools_installed_version=$(basename "${first_lib}" | sed 's/libortools.so.//')
            local or_tools_root
            or_tools_root=$(dirname "${first_lib}")
            local or_tools_install_dir
            or_tools_install_dir=$(realpath "${or_tools_root}/..")

            if [[ "${or_tools_installed_version}" == "${OR_TOOLS_VERSION_SMALL}" ]]; then
                CMAKE_PACKAGE_ROOT_ARGS+=" -D ortools_ROOT=${or_tools_install_dir} "
                OR_TOOLS_PATH=${or_tools_install_dir}

                INSTALL_SUMMARY+=("or-tools: system=${or_tools_installed_version}, required=${OR_TOOLS_VERSION_SMALL}, status=skipped")
                return
            else
                log "Found old OR-Tools version ${or_tools_installed_version}. Removing it."
                rm -rf "${or_tools_install_dir}"
            fi
        fi
    fi

    if [[ "$(uname -m)" == "aarch64" ]]; then
        (
            cd "${build_dir}"
            _execute "Cloning or-tools..." git clone --depth=1 -b "v${OR_TOOLS_VERSION_BIG}" https://github.com/google/or-tools.git
            cd or-tools
            local cmake_bin=${PREFIX:-/usr/local}/bin/cmake
            _execute "Configuring or-tools..." "${cmake_bin}" -S. -Bbuild -DBUILD_DEPS:BOOL=ON -DBUILD_EXAMPLES:BOOL=OFF -DBUILD_SAMPLES:BOOL=OFF -DBUILD_TESTING:BOOL=OFF -DCMAKE_INSTALL_PREFIX="${OR_TOOLS_PATH}" -DCMAKE_CXX_FLAGS="-w" -DCMAKE_C_FLAGS="-w"
            _execute "Building and installing or-tools..." "${cmake_bin}" --build build --config Release --target install -v -j "${NUM_THREADS}"
        )
    else
        (
            cd "${build_dir}"
            if [[ "${os_version}" == "rodete" ]]; then
                os_version=11
            fi
            local or_tools_file="or-tools_${arch}_${os}-${os_version}_cpp_v${OR_TOOLS_VERSION_SMALL}.tar.gz"
            _execute "Downloading or-tools..." wget "https://github.com/google/or-tools/releases/download/v${OR_TOOLS_VERSION_BIG}/${or_tools_file}"
            mkdir -p "${OR_TOOLS_PATH}"
            _execute "Extracting or-tools..." tar --strip 1 --dir "${OR_TOOLS_PATH}" -xf "${or_tools_file}"
        )
    fi

    INSTALL_SUMMARY+=("or-tools: system=${or_tools_installed_version}, required=${OR_TOOLS_VERSION_SMALL}, status=installed")

    CMAKE_PACKAGE_ROOT_ARGS+=" -D ortools_ROOT=$(realpath "${OR_TOOLS_PATH}") "
    rm -rf "${build_dir}"
}
# ==============================================================================
# Dependency Installation Modules
# ==============================================================================
# Each dependency will have its own dedicated function for installation and
# version management. This modular approach makes the script easier to
# maintain and extend.
_install_common_dev() {
    log "Install common development dependencies (-common or -all)"
    rm -rf "${BASE_DIR}"
    mkdir -p "${BASE_DIR}"
    if [[ -n "${PREFIX}" ]]; then
        mkdir -p "${PREFIX}"
    fi

    _install_cmake
    _install_bison
    _install_flex
    _install_pcre
    _install_swig
    _install_boost
    _install_eigen
    _install_cudd
    _install_cusp
    _install_lemon
    _install_spdlog
    _install_gtest

    if [[ "${EQUIVALENCE_DEPS}" == "yes" ]]; then
        _install_equivalence_deps
    fi

    if [[ "${CI}" == "yes" ]]; then
        _install_ninja
    fi

    if [[ -n ${PREFIX} ]]; then
        # Emit an environment setup script
        cat > "${PREFIX}/env.sh" <<EOF
if [ -n "\$ZSH_VERSION" ]; then
  depRoot="\$(dirname \$(readlink -f "\${(%):-%x}"))"
else
  depRoot="\$(dirname \$(readlink -f "\${BASH_SOURCE}"))"
fi

PATH=\${depRoot}/bin:\${PATH}
LD_LIBRARY_PATH=\${depRoot}/lib64:\${depRoot}/lib:\${LD_LIBRARY_PATH}
EOF
    fi
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
        automake autotools-dev binutils bison build-essential ccache clang \
        debhelper devscripts flex g++ gcc git groff lcov libbz2-dev libffi-dev libfl-dev \
        libgomp1 libomp-dev libpcre2-dev libreadline-dev pandoc \
        pkg-config python3-dev python3-click qt5-image-formats-plugins tcl tcl-dev tcl-tclreadline \
        tcllib unzip wget libyaml-cpp-dev zlib1g-dev tzdata

    local packages=()
    if _version_compare "$1" -ge "25.04"; then
        packages+=("libtcl8.6")
    else
        packages+=("libtcl")
    fi
    if _version_compare "$1" -ge "25.04"; then
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
        autoconf automake clang clang-devel gcc gcc-c++ gdb git glibc-devel \
        bzip2-devel libffi-devel libtool llvm llvm-devel llvm-libs make \
        pcre2-devel pkg-config pkgconf pkgconf-m4 pkgconf-pkg-config python3 \
        python3-devel python3-pip python3-click qt5-qtbase-devel qt5-qtcharts-devel \
        qt5-qtimageformats readline tcl-devel tcl-tclreadline \
        tcl-tclreadline-devel tcl-thread-devel tcllib wget yaml-cpp-devel \
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
            https://mirror.stream.centos.org/9-stream/AppStream/x86_64/os/Packages/readline-devel-8.1-4.el9.x86_64.rpm \
            https://rpmfind.net/linux/centos-stream/9-stream/AppStream/x86_64/os/Packages/tcl-devel-8.6.10-7.el9.x86_64.rpm
    fi

    local arch=amd64
    local pandoc_version="3.1.11.1"
    _execute "Downloading pandoc..." wget "https://github.com/jgm/pandoc/releases/download/${pandoc_version}/pandoc-${pandoc_version}-linux-${arch}.tar.gz"
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
        binutils clang gcc gcc11-c++ git groff gzip lcov libbz2-devel libffi-devel \
        libgomp1 libomp11-devel libpython3_6m1_0 libqt5-creator libqt5-qtbase \
        libqt5-qtstyleplugins libstdc++6-devel-gcc8 llvm pandoc \
        pcre2-devel pkg-config python3-devel python3-pip python3-click qimgv readline-devel tcl \
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
    _execute "Installing Homebrew packages..." brew install bison boost bzip2 cmake eigen flex fmt groff libomp or-tools pandoc pkg-config pyqt5 python spdlog tcl-tk zlib swig yaml-cpp
    _execute "Installing Python click..." pip3 install click
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
    if [[ "${debian_version}" == "rodete" ]]; then
        tcl_ver="8.6"
    fi
    _execute "Installing base packages..." apt-get -y install --no-install-recommends \
        automake autotools-dev binutils bison build-essential clang debhelper \
        devscripts flex g++ gcc git groff lcov libbz2-dev libffi-dev libfl-dev libgomp1 \
        libomp-dev libpcre2-dev libreadline-dev "libtcl${tcl_ver}" \
        pandoc pkg-config python3-dev python3-click qt5-image-formats-plugins tcl-dev tcl-tclreadline \
        tcllib unzip wget libyaml-cpp-dev zlib1g-dev tzdata

    if [[ "${debian_version}" == "10" ]]; then
        _execute "Installing Debian 10 specific packages..." apt-get install -y --no-install-recommends libpython3.7 libqt5charts5-dev qt5-default
    else
        local python_ver="3.8"
        if [[ "${debian_version}" == "rodete" ]]; then
            python_ver="3.12"
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
        apt-transport-https ca-certificates curl default-jdk gnupg python3 \
        python3-pip python3-pandas jq lsb-release parallel \
        software-properties-common time unzip zip

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
        _execute "Downloading LLVM install script..." wget https://apt.llvm.org/llvm.sh
        chmod +x llvm.sh
        _execute "Installing LLVM 16..." ./llvm.sh 16 all
    fi
}
_help() {
    cat <<EOF

Usage: $0 [OPTIONS]

Options:
  -all                        Install all dependencies (base and common). Requires privileged access.
  -base                       Install base dependencies using package managers. Requires privileged access.
  -common                     Install common dependencies.
  -eqy                        Install equivalence dependencies (yosys, eqy, sby).
  -prefix=DIR                 Install common dependencies in a user-specified directory.
  -local                      Install common dependencies in \${HOME}/.local.
  -ci                         Install dependencies required for CI.
  -nocert                     Disable certificate checks for downloads.
  -skip-system-or-tools       Skip searching for a system-installed or-tools library.
  -save-deps-prefixes=FILE    Save OpenROAD build arguments to FILE.
  -constant-build-dir         Use a constant build directory instead of a random one.
  -threads=<N>                Limit the number of compiling threads.
  -yosys-ver=<VERSION>        Specify a custom Yosys version. Used for ORFS.
  -verbose                    Show all output from build commands.
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
            -eqy) EQUIVALENCE_DEPS="yes" ;;
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
                ;;
            -nocert)
                warn "Security certificates for downloaded packages will not be checked."
                shopt -s expand_aliases
                alias wget="wget --no-check-certificate"
                export GIT_SSL_NO_VERIFY=true
                ;;
            -skip-system-or-tools) SKIP_SYSTEM_OR_TOOLS="true" ;;
            -save-deps-prefixes=*) SAVE_DEPS_PREFIXES="$(realpath "${1#*=}")" ;;
            -threads=*) NUM_THREADS="${1#*=}" ;;
            -yosys-ver=*) YOSYS_VERSION="${1#*=}" ;;
            *)
                echo "Unknown option: ${1}" >&2
                _help
                ;;
        esac
        shift 1
    done

    if [[ "${option}" == "none" ]]; then
        error "You must use one of: -all, -base, or -common."
    fi

    OR_TOOLS_PATH=${PREFIX:-"/opt/or-tools"}

    if [[ -z "${SAVE_DEPS_PREFIXES}" ]]; then
        local dir
        dir="$(dirname "$(readlink -f "$0")")"
        SAVE_DEPS_PREFIXES="${dir}/openroad_deps_prefixes.txt"
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
            if [[ "${option}" == "base" || "${option}" == "all" ]]; then
                _install_ubuntu_packages "${ubuntu_version}"
            fi
            if [[ "${CI}" == "yes" ]]; then
                _install_ci_packages "${ubuntu_version}"
            fi
            if [[ "${option}" == "common" || "${option}" == "all" ]]; then
                _install_common_dev
                local ubuntu_version_normalized=${ubuntu_version}
                if _version_compare "${ubuntu_version_normalized}" -ge "25.04"; then
                    # FIXME make do with or-tools for 24.10 until an official release for 25.04 is available
                    ubuntu_version_normalized="24.10"
                elif _version_compare "${ubuntu_version_normalized}" -ge "24.04"; then
                    ubuntu_version_normalized="24.04"
                elif _version_compare "${ubuntu_version_normalized}" -ge "22.04"; then
                    ubuntu_version_normalized="22.04"
                else
                    ubuntu_version_normalized="20.04"
                fi
                _install_or_tools "ubuntu" "${ubuntu_version_normalized}" "amd64" "${SKIP_SYSTEM_OR_TOOLS}"
                _install_abseil
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
            if [[ "${option}" == "base" || "${option}" == "all" ]]; then
                _install_rhel_packages "${rhel_version}"
            fi
            if [[ "${option}" == "common" || "${option}" == "all" ]]; then
                _install_common_dev
                local os_id
                os_id=$(awk -F= '/^ID/{print $2}' /etc/os-release | sed 's/"//g')
                local arch
                arch=$(uname -m)
                local or_tools_arch=${arch}
                local or_tools_distro=""
                local or_tools_version=""
                if [[ "${rhel_version}" == "8" ]]; then
                    or_tools_distro="AlmaLinux"
                    or_tools_version="8.10"
                    if [[ "${os_id}" != "almalinux" ]]; then
                        warn "Using AlmaLinux or-tools package for RHEL 8 compatible system."
                    fi
                elif [[ "${rhel_version}" == "9" ]]; then
                    or_tools_version="9"
                    if [[ "${arch}" == "x86_64" ]]; then
                        or_tools_arch="amd64"
                    fi
                    if [[ "${os_id}" == "almalinux" || "${os_id}" == "rocky" ]]; then
                        or_tools_distro="${os_id}"
                    else
                        or_tools_distro="rockylinux"
                        warn "Defaulting to rockylinux or-tools package for RHEL 9 compatible system."
                    fi
                fi
                _install_or_tools "${or_tools_distro}" "${or_tools_version}" "${or_tools_arch}" "${SKIP_SYSTEM_OR_TOOLS}"
                _install_abseil
            fi
            ;;
        "Darwin")
            _install_darwin_packages
            cat <<EOF

To install or run OpenROAD, update your path with:
    export PATH="\$(brew --prefix bison)/bin:\$(brew --prefix flex)/bin:\$(brew --prefix tcl-tk)/bin:\${PATH}"
    export CMAKE_PREFIX_PATH=\$(brew --prefix or-tools)
EOF
            ;;
        "openSUSE Leap")
            if [[ "${option}" == "base" || "${option}" == "all" ]]; then
                _install_opensuse_packages
            fi
            if [[ "${option}" == "common" || "${option}" == "all" ]]; then
                _install_common_dev
                _install_or_tools "opensuse" "leap" "amd64" "${SKIP_SYSTEM_OR_TOOLS}"
                _install_abseil
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
            if [[ "${option}" == "base" || "${option}" == "all" ]]; then
                _install_debian_packages "${debian_version}"
            fi
            if [[ "${option}" == "common" || "${option}" == "all" ]]; then
                _install_common_dev
                _install_or_tools "debian" "${debian_version}" "amd64" "${SKIP_SYSTEM_OR_TOOLS}"
                _install_abseil
            fi
            ;;
        *)
            error "Unsupported system: ${os}"
            ;;
    esac

    if [[ -n "${SAVE_DEPS_PREFIXES}" ]]; then
        mkdir -p "$(dirname "${SAVE_DEPS_PREFIXES}")"
        echo "${CMAKE_PACKAGE_ROOT_ARGS}" > "${SAVE_DEPS_PREFIXES}"
        if [[ $(id -u) == 0 && -n "${SUDO_USER+x}" ]]; then
            chown "${SUDO_USER}:$(id -gn "${SUDO_USER}")" "${SAVE_DEPS_PREFIXES}" 2>/dev/null || true
        fi
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
    echo "${BOLD}====================================================================================${NC}"
    printf "%-20s | %-20s | %-20s | %-10s\n" "Package" "System Version" "Required Version" "Status"
    printf "%-20s | %-20s | %-20s | %-10s\n" "--------------------" "--------------------" "--------------------" "----------"
    for summary_line in "${INSTALL_SUMMARY[@]}"; do
        # summary_line is like "Flex: system=2.6.4, required=2.6.4, status=skipped"
        local package
        package=$(echo "$summary_line" | cut -d':' -f1)
        local system_version
        system_version=$(echo "$summary_line" | sed -n 's/.*system=\([^,]*\).*/\1/p')
        local required_version
        required_version=$(echo "$summary_line" | sed -n 's/.*required=\([^,]*\).*/\1/p')
        local status
        status=$(echo "$summary_line" | sed -n 's/.*status=\([^,]*\).*/\1/p')
        local status_color=""
        if [[ "${status}" == "installed" ]]; then
            status_color="${GREEN}"
        elif [[ "${status}" == "skipped" ]]; then
            status_color="${YELLOW}"
        fi
        printf "%-20s | %-20s | %-20s | ${status_color}%-10s${NC}\n" "$package" "$system_version" "$required_version" "$status"
    done
    echo "${BOLD}====================================================================================${NC}"
}

main "$@"
