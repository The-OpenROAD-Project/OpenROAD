#!/bin/bash

set -euo pipefail

_installCommonDev() {
    lastDir="$(pwd)"
    # tools versions
    osName="linux"
    cmakeChecksum="b8d86f8c5ee990ae03c486c3631cee05"
    cmakeVersionBig=3.24
    cmakeVersionSmall=${cmakeVersionBig}.2
    swigVersionType="tag"
    swigVersion=4.1.0
    swigChecksum="794433378154eb61270a3ac127d9c5f3"
    boostVersionBig=1.80
    boostVersionSmall=${boostVersionBig}.0
    boostChecksum="077f074743ea7b0cb49c6ed43953ae95"
    eigenVersion=3.4
    lemonVersion=1.3.1
    lemonChecksum="e89f887559113b68657eca67cf3329b5"
    spdlogVersion=1.8.1

    # temp dir to download and compile
    baseDir=/tmp/installers
    mkdir -p "${baseDir}"

    # CMake
    if [[ -z $(cmake --version | grep ${cmakeVersionBig}) ]]; then
        cd "${baseDir}"
        wget https://cmake.org/files/v${cmakeVersionBig}/cmake-${cmakeVersionSmall}-${osName}-x86_64.sh
        md5sum -c <(echo "${cmakeChecksum}  cmake-${cmakeVersionSmall}-${osName}-x86_64.sh") || exit 1
        chmod +x cmake-${cmakeVersionSmall}-${osName}-x86_64.sh
        ./cmake-${cmakeVersionSmall}-${osName}-x86_64.sh --skip-license --prefix=/usr/local
    else
        echo "CMake already installed."
    fi

    # SWIG
    if [[ -z $(swig -version | grep ${swigVersion}) ]]; then
        cd "${baseDir}"
        tarName="rel-${swigVersion}.tar.gz"
        [[ ${swigVersionType} == "tag" ]] && tarName="v${swigVersion}.tar.gz"
        wget https://github.com/swig/swig/archive/${tarName}
        md5sum -c <(echo "${swigChecksum}  ${tarName}") || exit 1
        tar xfz ${tarName}
        cd swig-${tarName%%.tar*} || cd swig-${swigVersion}
        ./autogen.sh
        ./configure --prefix=/usr
        make -j $(nproc)
        make -j $(nproc) install
    else
        echo "Swig already installed."
    fi

    # boost
    if [[ -z $(grep "BOOST_LIB_VERSION \"${boostVersionBig//./_}\"" /usr/local/include/boost/version.hpp) ]]; then
        cd "${baseDir}"
        boostVersionUnderscore=${boostVersionSmall//./_}
        wget https://boostorg.jfrog.io/artifactory/main/release/${boostVersionSmall}/source/boost_${boostVersionUnderscore}.tar.gz
        md5sum -c <(echo "${boostChecksum}  boost_${boostVersionUnderscore}.tar.gz") || exit 1
        tar -xf boost_${boostVersionUnderscore}.tar.gz
        cd boost_${boostVersionUnderscore}
        ./bootstrap.sh
        ./b2 install --with-iostreams --with-test --with-serialization --with-system --with-thread -j $(nproc)
    else
        echo "Boost already installed."
    fi

    # eigen
    if [[ ! -d /usr/local/include/eigen3/ ]]; then
        cd "${baseDir}"
        git clone -b ${eigenVersion} https://gitlab.com/libeigen/eigen.git
        cd eigen
        cmake -B build .
        cmake --build build -j $(nproc) --target install
    else
        echo "Eigen already installed."
    fi

    # CUSP
    if [[ ! -d /usr/local/include/cusp/ ]]; then
        cd "${baseDir}"
        git clone -b cuda9 https://github.com/cusplibrary/cusplibrary.git
        cd cusplibrary
        cp -r ./cusp /usr/local/include/
    else
        echo "CUSP already installed."
    fi

    # lemon
    if [[ -z $(grep "LEMON_VERSION \"${lemonVersion}\"" /usr/local/include/lemon/config.h) ]]; then
        cd "${baseDir}"
        wget http://lemon.cs.elte.hu/pub/sources/lemon-${lemonVersion}.tar.gz
        md5sum -c <(echo "${lemonChecksum}  lemon-${lemonVersion}.tar.gz") || exit 1
        tar -xf lemon-${lemonVersion}.tar.gz
        cd lemon-${lemonVersion}
        cmake -B build .
        cmake --build build -j $(nproc) --target install
    else
        echo "Lemon already installed."
    fi

    # spdlog
    if [[ -z $(grep "PACKAGE_VERSION \"${spdlogVersion}\"" ${spdlogFolder}) ]]; then
        cd "${baseDir}"
        git clone -b "v${spdlogVersion}" https://github.com/gabime/spdlog.git
        cd spdlog
        cmake -B build .
        cmake --build build -j $(nproc) --target install
    else
        echo "spdlog already installed."
    fi

    cd "$lastDir"
    rm -rf "${baseDir}"
}

_installOrTools() {
    os=$1
    version=$2
    arch=$3
    orToolsVersionBig=9.4
    orToolsVersionSmall=${orToolsVersionBig}.1874
    orToolsFile=or-tools_${arch}_${os}-${version}_cpp_v${orToolsVersionSmall}.tar.gz
    wget https://github.com/google/or-tools/releases/download/v${orToolsVersionBig}/${orToolsFile}
    mkdir -p /opt/or-tools
    tar --strip 1 --dir /opt/or-tools -xf ${orToolsFile}
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
        build-essential \
        bison \
        flex \
        clang \
        g++ \
        gcc \
        git \
        lcov \
        libpcre2-dev \
        libpcre3-dev \
        python3-dev \
        libreadline-dev \
        tcl-dev \
        tcllib \
        wget \
        zlib1g-dev
}

_installUbuntuRuntime() {
    export DEBIAN_FRONTEND="noninteractive"
    apt-get -y update
    apt-get -y install tzdata
    apt-get install -y \
        binutils \
        libgomp1 \
        libpython3.8 \
        libtcl \
        qt5-image-formats-plugins \
        tcl-tclreadline

    if [[ $1 == 22.04 ]]; then
        apt-get install -y \
        qtbase5-dev \
        qtchooser \
        qt5-qmake \
        qtbase5-dev-tools
    else
        apt-get install -y qt5-default
    fi

    # need the strip "hack" above to run on docker
    strip --remove-section=.note.ABI-tag /usr/lib/x86_64-linux-gnu/libQt5Core.so
}

_installCentosCleanUp() {
    yum clean -y all
    rm -rf /var/lib/apt/lists/*
}

_installCentosDev() {
    if [[ -z $(yum list installed lcov) ]]; then
        yum install -y http://downloads.sourceforge.net/ltp/lcov-1.14-1.noarch.rpm
    fi
    if [[ -z $(yum list installed ius-release) ]]; then
        yum install -y https://repo.ius.io/ius-release-el7.rpm
    fi
    yum groupinstall -y "Development Tools"
    yum install -y centos-release-scl
    yum install -y \
        devtoolset-8 \
        devtoolset-8-libatomic-devel \
        libstdc++ \
        llvm-toolset-7.0 \
        llvm-toolset-7.0-libomp-devel \
        pcre-devel \
        pcre2-devel \
        readline-devel \
        tcl \
        tcl-devel \
        tcllib \
        tcl-tclreadline-devel \
        zlib-devel \
        wget
    yum install -y \
        python-devel \
        python36 \
        python36-devel \
        python36-pip
}

_installCentosRuntime() {
    yum update -y
    if [[ -z $(yum list installed epel-release) ]]; then
        yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
    fi
    yum install -y \
        libgomp \
        python36-libs \
        qt5-qtbase-devel \
        qt5-qtimageformats \
        tcl-tclreadline
    yum update -y
}

_installHomebrewPackage() {
    package=$1
    commit=$2
    url=https://raw.githubusercontent.com/Homebrew/homebrew-core/${commit}/Formula/${package}.rb
    curl -L ${url} > ${package}.rb

    if brew list $package &> /dev/null
        then
        # Homebrew is awful at letting you use the version you want if a newer
        # version is installed. The package must be completely removed to ensure
        # only the correct version is installed
        brew remove --force --ignore-dependencies $package
    fi

    # Must ignore dependencies to avoid automatic upgrade
    export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
    brew install --ignore-dependencies --formula ./${package}.rb
    brew pin ${package}

    # Cleanup
    rm ./${package}.rb
}

_installDarwin() {
    if ! command -v brew &> /dev/null
      then
      echo "Homebrew is not found. Please install homebrew before continuing."
      exit 1
      fi
    if ! xcode-select -p &> /dev/null
      then
      # xcode-select does not pause execution, so the user must handle it
      cat <<EOF
Xcode command line tools not installed.
Run the following command to install them:
  xcode-select --install
Then, rerun this script.
EOF
      exit 1
    fi
    brew install bison boost cmake eigen flex libomp pyqt5 python swig tcl-tk zlib

    # Lemon is not in the homebrew-core repo
    brew install The-OpenROAD-Project/lemon-graph/lemon-graph

    # Install fmt 8.1.1 because fmt 9 causes compile errors
    _installHomebrewPackage "fmt" "8643c850826702923f02d289e0f93a3b4433741b"
    # Install spdlog 1.9.2
    _installHomebrewPackage "spdlog" "0974b8721f2f349ed4a47a403323237e46f95ca0"
}

_help() {
    cat <<EOF

Usage: $0 -run[time]
       $0 -dev[elopment]

EOF
    exit "${1:-1}"
}

# default option
option="runtime"

# default values, can be overwritten by cmdline args
while [ "$#" -gt 0 ]; do
    case "${1}" in
        -h|-help)
            _help 0
            ;;
        -run|-runtime)
            option="runtime"
            ;;
        -dev|-development)
            option="dev"
            ;;
        *)
            echo "unknown option: ${1}" >&2
            _help
            ;;
    esac
    shift 1
done

platform="$(uname -s)"
case "${platform}" in
    "Linux" )
        if [[ -f /etc/os-release ]]; then
            os=$(awk -F= '/^NAME/{print $2}' /etc/os-release | sed 's/"//g')
        else
            os="Unidentified OS, could not find /etc/os-release."
        fi
        ;;
    "Darwin" )
        os="Darwin"
        ;;
    *)
        echo "${platform} is not supported" >&2
        echo "We only officially support Linux at the moment." >&2
        _help
        ;;
esac

case "${os}" in
    "CentOS Linux" )
        spdlogFolder="/usr/local/lib64/cmake/spdlog/spdlogConfigVersion.cmake"
        export spdlogFolder
        _installCentosRuntime
        if [[ "${option}" == "dev" ]]; then
            _installCentosDev
            _installCommonDev
        fi
        _installOrTools "centos" "7" "amd64"
        _installCentosCleanUp
        cat <<EOF
To enable GCC-8 or Clang-7 you need to run:
    source /opt/rh/devtoolset-8/enable
    source /opt/rh/llvm-toolset-7.0/enable
EOF
        ;;
    "Ubuntu" )
        version=$(awk -F= '/^VERSION_ID/{print $2}' /etc/os-release | sed 's/"//g')
        spdlogFolder="/usr/local/lib/cmake/spdlog/spdlogConfigVersion.cmake"
        export spdlogFolder
        _installUbuntuRuntime "${version}"
        if [[ "${option}" == "dev" ]]; then
            _installUbuntuDev
            _installCommonDev
        fi
        _installOrTools "ubuntu" "${version}" "amd64"
        _installUbuntuCleanUp
        ;;
    "Darwin" )
        _installDarwin
        _installOrTools "MacOsX" "12.5" $(uname -m)
        cat <<EOF

To install or run openroad, update your path with:
    export PATH="\$(brew --prefix bison)/bin:\$(brew --prefix flex)/bin:\$(brew --prefix tcl-tk)/bin:\$PATH"

You may wish to add this line to your .bashrc file
EOF
        ;;
    *)
        echo "unsupported system: ${os}" >&2
        echo "supported systems are CentOS 7 and Ubuntu 20.04" >&2
        _help
        ;;
esac
