#!/bin/bash

set -euo pipefail

_installCommonDev() {
    lastDir="$(pwd)"
    # tools versions
    cmakeVersionBig=3.14
    cmakeVersionSmall=${cmakeVersionBig}.0
    swigVersion=4.0.1
    boostVersion=1.72.0
    eigenVersion=3.3
    lemonVersion=1.3.1
    spdlogVersion=1.8.1

    # temp dir to download and compile
    baseDir=/tmp/installers
    mkdir -p "${baseDir}"

    # CMake
    cd "${baseDir}"
    wget https://cmake.org/files/v${cmakeVersionBig}/cmake-${cmakeVersionSmall}-Linux-x86_64.sh
    chmod +x cmake-${cmakeVersionSmall}-Linux-x86_64.sh
    ./cmake-${cmakeVersionSmall}-Linux-x86_64.sh --skip-license --prefix=/usr/local

    # SWIG
    cd "${baseDir}"
    wget https://github.com/swig/swig/archive/rel-${swigVersion}.tar.gz
    tar xfz rel-${swigVersion}.tar.gz
    cd swig-rel-${swigVersion}
    ./autogen.sh
    ./configure --prefix=/usr
    make -j $(nproc)
    make -j $(nproc) install
    rm -rf swig-rel-${swigVersion}

    # boost
    cd "${baseDir}"
    boost_version_underscore=${boostVersion//./_}
    wget https://dl.bintray.com/boostorg/release/${boostVersion}/source/boost_${boost_version_underscore}.tar.gz
    tar -xf boost_${boost_version_underscore}.tar.gz
    cd boost_${boost_version_underscore}
    ./bootstrap.sh
    ./b2 install --with-iostreams --with-test -j $(nproc)

    # eigen
    cd "${baseDir}"
    git clone -b ${eigenVersion} https://gitlab.com/libeigen/eigen.git
    cd eigen
    cmake -B build .
    cmake --build build -j $(nproc) --target install

    # lemon
    cd "${baseDir}"
    wget http://lemon.cs.elte.hu/pub/sources/lemon-${lemonVersion}.tar.gz
    tar -xf lemon-${lemonVersion}.tar.gz
    cd lemon-${lemonVersion}
    cmake -B build .
    cmake --build build -j $(nproc) --target install

    # spdlog
    cd "${baseDir}"
    git clone -b "v${spdlogVersion}" https://github.com/gabime/spdlog.git
    cd spdlog
    cmake -B build .
    cmake --build build -j $(nproc) --target install

    cd "$lastDir"
    rm -rf "${baseDir}"
}

_installUbuntuCleanUp() {
    apt-get autoclean -y
    apt-get autoremove -y
}

_installUbuntuDev() {
    export DEBIAN_FRONTEND="noninteractive"
    apt-get -y update
    apt-get -y install tzdata
    apt-get -y install \
        automake \
        autotools-dev \
        bison \
        flex \
        clang \
        g++ \
        gcc \
        git \
        lcov \
        libpcre3-dev \
        python3-dev \
        tcl-dev \
        tk-dev \
        wget \
        zlib1g-dev
}

_installUbuntuRuntime() {
    DEBIAN_FRONTEND="noninteractive"
    apt-get -y update
    apt-get -y install tzdata
    apt-get install -y \
        binutils \
        libgomp1 \
        libpython3.8 \
        libtcl \
        qt5-default \
        tcl-tclreadline
    # need the strip "hack" above to run on docker
    strip --remove-section=.note.ABI-tag /usr/lib/x86_64-linux-gnu/libQt5Core.so
}

_installCentosCleanUp() {
    yum clean -y all
    rm -rf /var/lib/apt/lists/*
}

_installCentosDev() {
    yum install -y http://downloads.sourceforge.net/ltp/lcov-1.14-1.noarch.rpm
    yum install -y https://repo.ius.io/ius-release-el7.rpm
    yum groupinstall -y "Development Tools"
    yum install -y centos-release-scl
    yum install -y \
        devtoolset-8 \
        devtoolset-8-libatomic-devel \
        libstdc++ \
        llvm-toolset-7.0 \
        llvm-toolset-7.0-libomp-devel \
        pcre-devel \
        tcl \
        tcl-devel \
        tcl-tclreadline-devel \
        tk \
        tk-devel \
        wget
    yum install -y \
        python36 \
        python36-devel \
        python36-pip
}

_installCentosRuntime() {
    yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
    yum install -y libgomp python36-libs qt5-qtbase-devel tcl-tclreadline
    yum update -y
}

_help() {
    cat <<EOF
usage: $0 -run[time]
       $0 -dev[elopment]
EOF
    exit "${1:-1}"
}

case "${1}" in
    -run|-runtime)
        option="runtime"
        ;;
    -dev|-development)
        option="dev"
        ;;
    *)
        echo "argument $1 not recognized" >&2
        _help
esac

platform="$(uname -s)"
case "${platform}" in
    "Linux" )
        if [[ -f /etc/os-release ]]; then
            os=$(awk -F= '/^NAME/{print $2}' /etc/os-release | sed 's/"//g')
        else
            os="UNIDENTIFIED (could not find /etc/os-release)"
        fi
        ;;
    *)
        echo "${platform} is not supported" >&2
        echo "we only support Linux at the moment" >&2
        _help
        ;;
esac

case "${os}" in
    "CentOS Linux" )
        _installCentosRuntime
        if [[ "${option}" == "dev" ]]; then
            _installCentosDev
            _installCommonDev
        fi
        _installCentosCleanUp
        ;;
    "Ubuntu" )
        _installUbuntuRuntime
        if [[ "${option}" == "dev" ]]; then
            _installUbuntuDev
            _installCommonDev
        fi
        _installUbuntuCleanUp
        ;;
    *)
        echo "unsupported system: ${os}" >&2
        echo "supported systems are CentOS 7 and Ubuntu 20.04" >&2
        _help
        ;;
esac
