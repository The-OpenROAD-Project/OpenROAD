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
    if [[ ! -z $PREFIX ]]; then mkdir -p ${PREFIX}; fi

    # CMake
    cmakePrefix=${PREFIX:-"/usr/local"}
    if [[ -z $(${cmakePrefix}/bin/cmake --version | grep ${cmakeVersionBig}) ]]; then
        cd "${baseDir}"
        wget https://cmake.org/files/v${cmakeVersionBig}/cmake-${cmakeVersionSmall}-${osName}-x86_64.sh
        md5sum -c <(echo "${cmakeChecksum}  cmake-${cmakeVersionSmall}-${osName}-x86_64.sh") || exit 1
        chmod +x cmake-${cmakeVersionSmall}-${osName}-x86_64.sh
        ./cmake-${cmakeVersionSmall}-${osName}-x86_64.sh --skip-license --prefix=${cmakePrefix}
    else
        echo "CMake already installed."
    fi

    # SWIG
    swigPrefix=${PREFIX:-"/usr"}
    if [[ -z $(${swigPrefix}/bin/swig -version | grep ${swigVersion}) ]]; then
        cd "${baseDir}"
        tarName="rel-${swigVersion}.tar.gz"
        [[ ${swigVersionType} == "tag" ]] && tarName="v${swigVersion}.tar.gz"
        wget https://github.com/swig/swig/archive/${tarName}
        md5sum -c <(echo "${swigChecksum}  ${tarName}") || exit 1
        tar xfz ${tarName}
        cd swig-${tarName%%.tar*} || cd swig-${swigVersion}
        ./autogen.sh
        ./configure --prefix=${swigPrefix}
        make -j $(nproc)
        make -j $(nproc) install
    else
        echo "Swig already installed."
    fi

    # boost
    boostPrefix=${PREFIX:-"/usr/local/include"}
    if [[ -z $(grep "BOOST_LIB_VERSION \"${boostVersionBig//./_}\"" ${boostPrefix}/boost/version.hpp) ]]; then
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
    eigenPrefix=${PREFIX:-"/usr/local/include"}
    if [[ ! -d ${eigenPrefix}/eigen3/ ]]; then
        cd "${baseDir}"
        git clone -b ${eigenVersion} https://gitlab.com/libeigen/eigen.git
        cd eigen
        ${cmakePrefix}/bin/cmake -B build .
        ${cmakePrefix}/bin/cmake --build build -j $(nproc) --target install
    else
        echo "Eigen already installed."
    fi

    # CUSP
    cuspPrefix=${PREFIX:-"/usr/local/include"}
    if [[ ! -d ${cuspPrefix}/cusp/ ]]; then
        cd "${baseDir}"
        git clone -b cuda9 https://github.com/cusplibrary/cusplibrary.git
        cd cusplibrary
        cp -r ./cusp ${cuspPrefix}
    else
        echo "CUSP already installed."
    fi

    # lemon
    lemonPrefix=${PREFIX:-"/usr/local/include"}
    if [[ -z $(grep "LEMON_VERSION \"${lemonVersion}\"" ${lemonPrefix}/lemon/config.h) ]]; then
        cd "${baseDir}"
        wget http://lemon.cs.elte.hu/pub/sources/lemon-${lemonVersion}.tar.gz
        md5sum -c <(echo "${lemonChecksum}  lemon-${lemonVersion}.tar.gz") || exit 1
        tar -xf lemon-${lemonVersion}.tar.gz
        cd lemon-${lemonVersion}
        ${cmakePrefix}/bin/cmake -B build .
        ${cmakePrefix}/bin/cmake --build build -j $(nproc) --target install
    else
        echo "Lemon already installed."
    fi

    # spdlog
    if [[ -z $(grep "PACKAGE_VERSION \"${spdlogVersion}\"" ${spdlogFolder}) ]]; then
        cd "${baseDir}"
        git clone -b "v${spdlogVersion}" https://github.com/gabime/spdlog.git
        cd spdlog
        ${cmakePrefix}/bin/cmake -B build .
        ${cmakePrefix}/bin/cmake --build build -j $(nproc) --target install
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
        zlib1g-dev \
        libomp-dev
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

_installRHELCleanUp() {
    yum clean -y all
    rm -rf /var/lib/apt/lists/*
}

_installRHELDev() {
    yum -y install \
        autoconf \
        automake \
        gcc \
        gcc-c++ \
        gdb \
        glibc-devel \
        libtool	\
        make \
        pkgconf \
        pkgconf-m4 \
        pkgconf-pkg-config \
        redhat-rpm-config \
        rpm-build \
        wget \
        git \
        llvm7.0 \
        llvm7.0-libs \
        llvm7.0-devel \
        pcre-devel \
        pcre2-devel \
        tcl-tclreadline-devel \
        readline \
        tcllib \
        tcl-tclreadline-devel \
        tcl-thread-devel \
        zlib-devel \
        python3 \
        python3-pip \
        python3-devel \
        clang \
        clang-devel

    yum install -y \
        http://repo.okay.com.mx/centos/8/x86_64/release/bison-3.0.4-10.el8.x86_64.rpm \
        https://forensics.cert.org/centos/cert/7/x86_64/flex-2.6.1-9.el7.x86_64.rpm \
        https://vault.centos.org/centos/8/BaseOS/x86_64/os/Packages/tcl-devel-8.6.8-2.el8.i686.rpm
}

_installRHELRuntime() {
    yum -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm

    yum -y update
    yum -y install \
        tzdata \
        binutils \
        libgomp \
        python3-libs \
        tcl \
        tcl-tclreadline \
        qt5-srpm-macros.noarch
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
    if [[ -z $(yum list installed epel-release) ]]; then
        yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
    fi
    yum update -y
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

_installDebianCleanUp() {
    apt-get autoclean -y
    apt-get autoremove -y
}

_installDebianDev() {
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
        zlib1g-dev \
        libomp-dev
}

_installDebianRuntime() {
    export DEBIAN_FRONTEND="noninteractive"
    apt-get -y update
    apt-get -y install tzdata
    apt-get install -y \
        binutils \
        libgomp1 \
        libtcl \
        qt5-image-formats-plugins \
        tcl-tclreadline
    
    if [[ $1 == 10 ]]; then
        apt-get install -y \
            libpython3.7 \
            qt5-default
    else
        apt-get install -y \
            libpython3.8 \
            qtbase5-dev \
            qtchooser \
            qt5-qmake \
            qtbase5-dev-tools
    fi
}

_help() {
    cat <<EOF

Usage: $0 -run[time]
       $0 -dev[elopment]
       $0 -prefix=DIR
       $0 -local

EOF
    exit "${1:-1}"
}

# default option
option="runtime"
PREFIX=""

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
        -local)
            export PREFIX="$HOME/.local"
            ;;
        -prefix=*)
            export PREFIX="$(echo $1 | sed -e 's/^[^=]*=//g')"
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
    "Red Hat Enterprise Linux")
        spdlogFolder="/usr/local/lib64/cmake/spdlog/spdlogConfigVersion.cmake"
        export spdlogFolder
        _installRHELRuntime
        if [[ "${option}" == "dev" ]]; then
            _installRHELDev
            _installCommonDev
        fi
        _installOrTools "centos" "8" "amd64"
        _installRHELCleanUp
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
    "Debian GNU/Linux" )
        version=$(awk -F= '/^VERSION_ID/{print $2}' /etc/os-release | sed 's/"//g')
        spdlogFolder="/usr/local/lib/cmake/spdlog/spdlogConfigVersion.cmake"
        export spdlogFolder
        _installDebianRuntime "${version}"
        if [[ "${option}" == "dev" ]]; then
            _installDebianDev
            _installCommonDev
        fi
        _installOrTools "debian" "${version}" "amd64"
        _installDebianCleanUp
        ;;
    *)
        echo "unsupported system: ${os}" >&2
        echo "supported systems are CentOS 7 and Ubuntu 20.04" >&2
        _help
        ;;
esac

if [[ ! -z ${PREFIX} ]]; then
            cat <<EOF
To use cmake, set cmake as an alias:
    alias cmake='${PREFIX}/bin/cmake' 
    or  run
    echo export PATH=${PREFIX}/bin:'$PATH' >> ~/.bash_profile
    source ~/.bash_profile
EOF
fi

