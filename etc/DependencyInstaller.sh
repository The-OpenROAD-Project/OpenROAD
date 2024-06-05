#!/usr/bin/env bash

set -euo pipefail

_versionCompare() {
    local a b IFS=. ; set -f
    printf -v a %08d $1; printf -v b %08d $3
    test $a "$2" $b
}

_equivalenceDeps() {
    yosysVersion=yosys-0.33
    eqyVersion=8327ac7

    # yosys
    yosysPrefix=${PREFIX:-"/usr/local"}
    if [[ ! $(command -v yosys) || ! $(command -v yosys-config)  ]]; then (
        if [[ -f /opt/rh/llvm-toolset-7.0/enable ]]; then
            source /opt/rh/llvm-toolset-7.0/enable
        fi
        cd "${baseDir}"
        git clone --depth=1 -b "${yosysVersion}" --recursive https://github.com/YosysHQ/yosys
        cd yosys
        # use of no-register flag is required for some compilers,
        # e.g., gcc and clang fron RHEL8
        make -j $(nproc) PREFIX="${yosysPrefix}" ABC_ARCHFLAGS=-Wno-register
        make install
    ) fi

    # eqy
    eqyPrefix=${PREFIX:-"/usr/local"}
    if ! command -v eqy &> /dev/null; then (
        if [[ -f /opt/rh/llvm-toolset-7.0/enable ]]; then
            source /opt/rh/llvm-toolset-7.0/enable
        fi
        cd "${baseDir}"
        git clone --recursive https://github.com/YosysHQ/eqy
        cd eqy
        git checkout ${eqyVersion}
        export PATH="${yosysPrefix}/bin:${PATH}"
        make -j $(nproc) PREFIX="${eqyPrefix}"
        make install PREFIX="${eqyPrefix}"
    )
    fi

    # sby
    sbyPrefix=${PREFIX:-"/usr/local"}
    if ! command -v sby &> /dev/null; then (
        if [[ -f /opt/rh/llvm-toolset-7.0/enable ]]; then
            source /opt/rh/llvm-toolset-7.0/enable
        fi
        cd "${baseDir}"
        git clone --depth=1 -b ${yosysVersion} --recursive https://github.com/YosysHQ/sby
        cd sby
        export PATH="${eqyPrefix}/bin:${PATH}"
        make -j $(nproc) PREFIX="${sbyPrefix}" install
    )
    fi
}

_installCommonDev() {
    lastDir="$(pwd)"
    # tools versions
    osName="linux"
    cmakeChecksum="b8d86f8c5ee990ae03c486c3631cee05"
    cmakeVersionBig=3.24
    cmakeVersionSmall=${cmakeVersionBig}.2
    pcreVersion=10.42
    pcreChecksum="37d2f77cfd411a3ddf1c64e1d72e43f7"
    swigVersion=4.1.0
    swigChecksum="794433378154eb61270a3ac127d9c5f3"
    boostVersionBig=1.80
    boostVersionSmall=${boostVersionBig}.0
    boostChecksum="077f074743ea7b0cb49c6ed43953ae95"
    eigenVersion=3.4
    lemonVersion=1.3.1
    spdlogVersion=1.8.1

    rm -rf "${baseDir}"
    mkdir -p "${baseDir}"
    if [[ ! -z "${PREFIX}" ]]; then
        mkdir -p "${PREFIX}"
    fi

    # CMake
    cmakePrefix=${PREFIX:-"/usr/local"}
    cmakeBin=${cmakePrefix}/bin/cmake
    if [[ ! -f ${cmakeBin} || -z $(${cmakeBin} --version | grep ${cmakeVersionBig}) ]]; then
        cd "${baseDir}"
        wget https://cmake.org/files/v${cmakeVersionBig}/cmake-${cmakeVersionSmall}-${osName}-x86_64.sh
        md5sum -c <(echo "${cmakeChecksum} cmake-${cmakeVersionSmall}-${osName}-x86_64.sh") || exit 1
        chmod +x cmake-${cmakeVersionSmall}-${osName}-x86_64.sh
        ./cmake-${cmakeVersionSmall}-${osName}-x86_64.sh --skip-license --prefix=${cmakePrefix}
    else
        echo "CMake already installed."
    fi

    # SWIG
    swigPrefix=${PREFIX:-"/usr/local"}
    swigBin=${swigPrefix}/bin/swig
    if [[ ! -f ${swigBin} || -z $(${swigBin} -version | grep ${swigVersion}) ]]; then
        cd "${baseDir}"
        tarName="v${swigVersion}.tar.gz"
        wget https://github.com/swig/swig/archive/${tarName}
        md5sum -c <(echo "${swigChecksum} ${tarName}") || exit 1
        tar xfz ${tarName}
        cd swig-${tarName%%.tar*} || cd swig-${swigVersion}

        # Check if pcre2 is installed
        if [[ -z $(pcre2-config --version) ]]; then
          tarName="pcre2-${pcreVersion}.tar.gz"
          wget https://github.com/PCRE2Project/pcre2/releases/download/pcre2-${pcreVersion}/${tarName}
          md5sum -c <(echo "${pcreChecksum} ${tarName}") || exit 1
          ./Tools/pcre-build.sh
        fi
        ./autogen.sh
        ./configure --prefix=${swigPrefix}
        make -j $(nproc)
        make -j $(nproc) install
    else
        echo "Swig already installed."
    fi

    # boost
    boostPrefix=${PREFIX:-"/usr/local"}
    if [[ -z $(grep "BOOST_LIB_VERSION \"${boostVersionBig//./_}\"" ${boostPrefix}/include/boost/version.hpp) ]]; then
        cd "${baseDir}"
        boostVersionUnderscore=${boostVersionSmall//./_}
        wget https://sourceforge.net/projects/boost/files/boost/${boostVersionSmall}/boost_${boostVersionUnderscore}.tar.gz
        # wget https://boostorg.jfrog.io/artifactory/main/release/${boostVersionSmall}/source/boost_${boostVersionUnderscore}.tar.gz
        md5sum -c <(echo "${boostChecksum}  boost_${boostVersionUnderscore}.tar.gz") || exit 1
        tar -xf boost_${boostVersionUnderscore}.tar.gz
        cd boost_${boostVersionUnderscore}
        ./bootstrap.sh --prefix="${boostPrefix}"
        ./b2 install --with-iostreams --with-test --with-serialization --with-system --with-thread -j $(nproc)
    else
        echo "Boost already installed."
    fi

    # eigen
    eigenPrefix=${PREFIX:-"/usr/local"}
    if [[ ! -d ${eigenPrefix}/include/eigen3 ]]; then
        cd "${baseDir}"
        git clone --depth=1 -b ${eigenVersion} https://gitlab.com/libeigen/eigen.git
        cd eigen
        ${cmakePrefix}/bin/cmake -DCMAKE_INSTALL_PREFIX="${eigenPrefix}" -B build .
        ${cmakePrefix}/bin/cmake --build build -j $(nproc) --target install
    else
        echo "Eigen already installed."
    fi

    # CUSP
    cuspPrefix=${PREFIX:-"/usr/local/include"}
    if [[ ! -d ${cuspPrefix}/cusp/ ]]; then
        cd "${baseDir}"
        git clone --depth=1 -b cuda9 https://github.com/cusplibrary/cusplibrary.git
        cd cusplibrary
        cp -r ./cusp ${cuspPrefix}
    else
        echo "CUSP already installed."
    fi

    # lemon
    lemonPrefix=${PREFIX:-"/usr/local"}
    if [[ -z $(grep "LEMON_VERSION \"${lemonVersion}\"" ${lemonPrefix}/include/lemon/config.h) ]]; then
        cd "${baseDir}"
        git clone --depth=1 -b ${lemonVersion} https://github.com/The-OpenROAD-Project/lemon-graph.git
        cd lemon-graph
        ${cmakePrefix}/bin/cmake -DCMAKE_INSTALL_PREFIX="${lemonPrefix}" -B build .
        ${cmakePrefix}/bin/cmake --build build -j $(nproc) --target install
    else
        echo "Lemon already installed."
    fi

    # spdlog
    spdlogPrefix=${PREFIX:-"/usr/local"}
    if [[ ! -d ${spdlogPrefix}/include/spdlog ]]; then
        cd "${baseDir}"
        git clone --depth=1 -b "v${spdlogVersion}" https://github.com/gabime/spdlog.git
        cd spdlog
        ${cmakePrefix}/bin/cmake -DCMAKE_INSTALL_PREFIX="${spdlogPrefix}" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DSPDLOG_BUILD_EXAMPLE=OFF -B build .
        ${cmakePrefix}/bin/cmake --build build -j $(nproc) --target install
    else
        echo "spdlog already installed."
    fi

    if [[ ${equivalenceDeps} == "yes" ]]; then
        _equivalenceDeps
    fi

    if [[ ${CI} == "yes" ]]; then
        # ninja
        ninjaCheckSum="817e12e06e2463aeb5cb4e1d19ced606"
        ninjaVersion=1.10.2
        ninjaPrefix=${PREFIX:-"/usr/local"}
        ninjaBin=${ninjaPrefix}/bin/ninja
        if [[ ! -d ${ninjaBin} ]]; then
            cd "${baseDir}"
            wget -O ninja-linux.zip https://github.com/ninja-build/ninja/releases/download/v${ninjaVersion}/ninja-linux.zip
            md5sum -c <(echo "${ninjaCheckSum} ninja-linux.zip") || exit 1
            unzip -o ninja-linux.zip -d ${ninjaPrefix}/bin/
            chmod +x ${ninjaBin}
        else
            echo "ninja already installed."
        fi
    fi

    cd "${lastDir}"
    rm -rf "${baseDir}"

    if [[ ! -z ${PREFIX} ]]; then
      # Emit an environment setup script
      cat > ${PREFIX}/env.sh <<EOF
depRoot="\$(dirname \$(readlink -f "\${BASH_SOURCE[0]}"))"
PATH=\${depRoot}/bin:\${PATH}
LD_LIBRARY_PATH=\${depRoot}/lib64:\${depRoot}/lib:\${LD_LIBRARY_PATH}
EOF
    fi
}

_installOrTools() {
    os=$1
    version=$2
    arch=$3
    orToolsVersionBig=9.5
    orToolsVersionSmall=${orToolsVersionBig}.2237

    rm -rf "${baseDir}"
    mkdir -p "${baseDir}"
    if [[ ! -z "${PREFIX}" ]]; then mkdir -p "${PREFIX}"; fi
    cd "${baseDir}"

    orToolsFile=or-tools_${arch}_${os}-${version}_cpp_v${orToolsVersionSmall}.tar.gz
    wget https://github.com/google/or-tools/releases/download/v${orToolsVersionBig}/${orToolsFile}
    orToolsPath=${PREFIX:-"/opt/or-tools"}
    if command -v brew &> /dev/null; then
        orToolsPath="$(brew --prefix or-tools)"
    fi
    mkdir -p ${orToolsPath}
    tar --strip 1 --dir ${orToolsPath} -xf ${orToolsFile}
    rm -rf ${baseDir}
}

_installUbuntuCleanUp() {
    apt-get autoclean -y
    apt-get autoremove -y
}

_installUbuntuPackages() {
    export DEBIAN_FRONTEND="noninteractive"
    apt-get -y update
    apt-get -y install tzdata
    apt-get -y install \
        automake \
        autotools-dev \
        binutils \
        bison \
        build-essential \
        clang \
        debhelper \
        devscripts \
        flex \
        g++ \
        gcc \
        git \
        groff \
        lcov \
        libffi-dev \
        libgomp1 \
        libomp-dev \
        libpcre2-dev \
        libpcre3-dev \
        libreadline-dev \
        libtcl \
        pandoc \
        python3-dev \
        qt5-image-formats-plugins \
        tcl \
        tcl-dev \
        tcl-tclreadline \
        tcllib \
        wget \
        zlib1g-dev \
        ccache \

    if _versionCompare $1 -ge 22.10; then
        apt-get install -y \
            libpython3.11 \
            qt5-qmake \
            qtbase5-dev \
            qtbase5-dev-tools \
            libqt5charts5-dev \
            qtchooser
    elif [[ $1 == 22.04 ]]; then
        apt-get install -y \
            libpython3.8 \
            qt5-qmake \
            qtbase5-dev \
            qtbase5-dev-tools \
            libqt5charts5-dev \
            qtchooser
    else
        apt-get install -y \
            libpython3.8 \
            libqt5charts5-dev \
            qt5-default
    fi
}

_installRHELCleanUp() {
    yum clean -y all
    rm -rf /var/lib/apt/lists/*
}

_installRHELPackages() {
    arch=amd64
    version=3.1.11.1

    yum -y update
    if [[ $(yum repolist | egrep -c "rhel-8-for-x86_64-appstream-rpms") -eq 0 ]]; then
        yum -y install http://mirror.centos.org/centos/8-stream/BaseOS/x86_64/os/Packages/centos-gpg-keys-8-6.el8.noarch.rpm
        yum -y install http://mirror.centos.org/centos/8-stream/BaseOS/x86_64/os/Packages/centos-stream-repos-8-6.el8.noarch.rpm
        rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-centosofficial
    fi
    yum -y install tzdata
    yum -y install redhat-rpm-config rpm-build
    yum -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm
    yum -y install \
        autoconf \
        automake \
        clang \
        clang-devel \
        gcc \
        gcc-c++ \
        gdb \
        git \
        glibc-devel \
        libtool \
        libffi-devel \
        llvm7.0 \
        llvm7.0-devel \
        llvm7.0-libs \
        make \
        pcre-devel \
        pcre2-devel \
        pkgconf \
        pkgconf-m4 \
        pkgconf-pkg-config \
        python3 \
        python3-devel \
        python3-pip \
        qt5-qtbase-devel \
        qt5-qtimageformats \
        readline \
        readline-devel \
        tcl-devel \
        tcl-tclreadline \
        tcl-tclreadline-devel \
        tcl-thread-devel \
        tcllib \
        wget \
        zlib-devel

    yum install -y \
        http://repo.okay.com.mx/centos/8/x86_64/release/bison-3.0.4-10.el8.x86_64.rpm \
        https://forensics.cert.org/centos/cert/7/x86_64/flex-2.6.1-9.el7.x86_64.rpm

    wget https://github.com/jgm/pandoc/releases/download/${version}/pandoc-${version}-linux-${arch}.tar.gz &&\
    tar xvzf pandoc-${version}-linux-${arch}.tar.gz --strip-components 1 -C /usr/local/ &&\
    rm -rf pandoc-${version}-linux-${arch}.tar.gz
}

_installCentosCleanUp() {
    yum clean -y all
    rm -rf /var/lib/apt/lists/*
}

_installCentosPackages() {
    yum update -y
    yum install -y tzdata
    yum groupinstall -y "Development Tools"
    if ! command -v lcov &> /dev/null; then
        yum install -y http://downloads.sourceforge.net/ltp/lcov-1.14-1.noarch.rpm
    fi
    if ! command -v yum list installed ius-release &> /dev/null; then
        yum install -y https://repo.ius.io/ius-release-el7.rpm
    fi
    if ! command -v yum list installed epel-release &> /dev/null; then
        yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
    fi
    yum install -y centos-release-scl
    yum install -y \
        devtoolset-8 \
        devtoolset-8-libatomic-devel \
        groff \
        libffi-devel \
        libgomp \
        libstdc++ \
        llvm-toolset-7.0 \
        llvm-toolset-7.0-libomp-devel \
        pandoc \
        pcre-devel \
        pcre2-devel \
        python-devel \
        python36 \
        python36-devel \
        python36-libs \
        python36-pip \
        qt5-qtbase-devel \
        qt5-qtimageformats \
        readline-devel \
        rh-python38-python \
        rh-python38-python-libs \
        rh-python38-python-pip \
        rh-python38-scldevel \
        tcl \
        tcl-devel \
        tcl-tclreadline \
        tcl-tclreadline-devel \
        tcllib \
        wget \
        ccache \
        zlib-devel
    }

_installOpenSuseCleanUp() {
    zypper -n clean --all
    zypper -n packages --unneeded \
        | awk -F'|' 'NR==0 || NR==1 || NR==2 || NR==3 || NR==4 {next} {print $3}' \
        | grep -v Name \
        | xargs -r zypper -n remove --clean-deps;
}

_installOpenSusePackages() {
    zypper refresh
    zypper -n update
    zypper -n install -t pattern devel_basis
    zypper -n install \
        binutils \
        clang \
        gcc \
        gcc11-c++ \
        git \
        groff \
        gzip \
        lcov \
        libffi-devel \
        libgomp1 \
        libomp11-devel \
        libpython3_6m1_0 \
        libqt5-creator \
        libqt5-qtbase \
        libqt5-qtstyleplugins \
        libstdc++6-devel-gcc8 \
        llvm \
        pandoc \
        pcre-devel \
        pcre2-devel \
        python3-devel \
        python3-pip \
        qimgv \
        readline-devel \
        tcl \
        tcl-devel \
        tcllib \
        wget \
        zlib-devel
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 50
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 50
}

_installHomebrewPackage() {
    package=$1
    commit=$2
    dir=$3
    url=https://raw.githubusercontent.com/Homebrew/homebrew-core/${commit}/Formula/${dir}${package}.rb
    curl -L ${url} > ${package}.rb

    if brew list "${package}" &> /dev/null; then
        # Homebrew is awful at letting you use the version you want if a newer
        # version is installed. The package must be completely removed to ensure
        # only the correct version is installed
        brew remove --force --ignore-dependencies "${package}"
    fi

    # Must ignore dependencies to avoid automatic upgrade
    export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
    brew install --ignore-dependencies --formula ./${package}.rb
    brew pin ${package}

    # Cleanup
    rm ./${package}.rb
}

_installDarwin() {
    if ! command -v brew &> /dev/null; then
      echo "Homebrew is not found. Please install homebrew before continuing."
      exit 1
      fi
    if ! xcode-select -p &> /dev/null; then
      # xcode-select does not pause execution, so the user must handle it
      cat <<EOF
Xcode command line tools not installed.
Run the following command to install them:
  xcode-select --install
Then, rerun this script.
EOF
      exit 1
    fi
    brew install bison boost cmake eigen flex groff libomp or-tools pandoc pyqt5 python tcl-tk zlib

    # Some systems neeed this to correclty find OpenMP package during build
    brew link --force libomp

    # Lemon is not in the homebrew-core repo
    brew install The-OpenROAD-Project/lemon-graph/lemon-graph

    # Install fmt 8.1.1 because fmt 9 causes compile errors
    _installHomebrewPackage "fmt" "8643c850826702923f02d289e0f93a3b4433741b" ""
    # Install spdlog 1.9.2
    _installHomebrewPackage "spdlog" "0974b8721f2f349ed4a47a403323237e46f95ca0" ""
    # Install swig 4.1.1
    _installHomebrewPackage "swig" "c83c8aaa6505c3ea28c35bc45a54234f79e46c5d" "s/"
}

_installDebianCleanUp() {
    apt-get autoclean -y
    apt-get autoremove -y
}

_installDebianPackages() {
    export DEBIAN_FRONTEND="noninteractive"
    apt-get -y update
    apt-get -y install tzdata
    apt-get -y install \
        automake \
        autotools-dev \
        binutils \
        bison \
        build-essential \
        clang \
        debhelper \
        devscripts \
        flex \
        g++ \
        gcc \
        git \
        groff \
        lcov \
        libgomp1 \
        libomp-dev \
        libpcre2-dev \
        libpcre3-dev \
        libreadline-dev \
        libtcl \
        pandoc \
        python3-dev \
        qt5-image-formats-plugins \
        tcl-dev \
        tcl-tclreadline \
        tcllib \
        wget \
        zlib1g-dev

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

_installCI() {
    apt-get -y update

    #docker
    apt install -y \
        apt-transport-https \
        ca-certificates \
        curl \
        software-properties-common
    # apt-get -y install ca-certificates curl
    # install -m 0755 -d /etc/apt/keyrings
    curl -fsSL https://download.docker.com/linux/ubuntu/gpg | gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg

    echo \
    "deb [arch=amd64 signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu \
    $(lsb_release -cs) stable" | tee /etc/apt/sources.list.d/docker.list > /dev/null
    apt-get -y update
    apt-get -y install docker-ce docker-ce-cli containerd.io
}

_checkIsLocal() {
    if [[ "${isLocal}" == "true" ]]; then
        echo "ERROR: cannot install base packages locally; you need privileged access." >&2
        echo "Hint: -local is only used with -common to install common packages." >&2
        exit 1
    fi
}

_help() {
    cat <<EOF

Usage: $0
                                # Installs all of OpenROAD's dependencies no
                                #     need to run -base or -common. Requires
                                #     privileged access.
                                #
       $0 -base
                                # Installs OpenROAD's dependencies using
                                #     package managers (-common must be
                                #     executed in another command).
       $0 -common
                                # Installs OpenROAD's common dependencies
                                #     (-base must be executed in another
                                #     command).
       $0 -prefix=DIR
                                # Installs common dependencies in an existing
                                #     user-specified directory. Only used
                                #     with -common. This flag cannot be used
                                #     with sudo or with root access.
       $0 -local
                                # Installs common dependencies in
                                #    "$HOME/.local". Only used with
                                #    -common. This flag cannot be used with
                                #    sudo or with root access.
       $0 -ci
                                # Installs dependencies required to run CI

EOF
    exit "${1:-1}"
}

# Default values
PREFIX=""
option="all"
isLocal="false"
equivalenceDeps="no"
CI="no"
# temp dir to download and compile
baseDir=$(mktemp -d /tmp/DependencyInstaller-XXXXXX)

# default values, can be overwritten by cmdline args
while [ "$#" -gt 0 ]; do
    case "${1}" in
        -h|-help)
            _help 0
            ;;
        -run|-runtime)
            echo "The use of this flag is deprecated and will be removed soon."
            ;;
        -dev|-development)
            echo "The use of this flag is deprecated and will be removed soon."
            ;;
        -base)
            if [[ "${option}" != "all" ]]; then
                echo "WARNING: previous argument -${option} will be overwritten with -base." >&2
            fi
            option="base"
            ;;
        -common)
            if [[ "${option}" != "all" ]]; then
                echo "WARNING: previous argument -${option} will be overwritten with -common." >&2
            fi
            option="common"
            ;;
        -eqy)
            equivalenceDeps="yes"
            ;;
        -ci)
            CI="yes"
            ;;
        -local)
            if [[ $(id -u) == 0 ]]; then
                echo "ERROR: cannot install locally (i.e., use -local) if you are root or using sudo." >&2
                exit 1
            fi
            if [[ ! -z ${PREFIX} ]]; then
                echo "WARNING: previous argument -prefix will be overwritten with -local"
            fi
            export PREFIX="${HOME}/.local"
            export isLocal="true"
            ;;
        -prefix=*)
            if [[ ! -z ${PREFIX} ]]; then
                echo "WARNING: previous argument -local will be overwritten with -prefix"
                export isLocal="false"
            fi
            export PREFIX="$(realpath $(echo $1 | sed -e 's/^[^=]*=//g'))"
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
        if [[ $(id -u) == 0 ]]; then>&2
            echo "ERROR: cannot install on macOS if you are root or using sudo (not recommended for brew)." >&2
            exit 1
        fi
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
        if [[ ${CI} == "yes" ]]; then
            echo "WARNING: Installing CI dependencies is only supported on Ubuntu 22.04" >&2
        fi
        if [[ "${option}" == "base" || "${option}" == "all" ]]; then
            _checkIsLocal
            _installCentosPackages
            _installCentosCleanUp
        fi
        if [[ "${option}" == "common" || "${option}" == "all" ]]; then
            _installCommonDev
            _installOrTools "centos" "7" "amd64"
        fi
        cat <<EOF
To enable Python 3.8 (required for eqy) you need to run:
    source /opt/rh/rh-python38/enable
To enable GCC-8 or Clang-7 you need to run:
    source /opt/rh/devtoolset-8/enable
    source /opt/rh/llvm-toolset-7.0/enable
EOF
        ;;
    "Ubuntu" )
        version=$(awk -F= '/^VERSION_ID/{print $2}' /etc/os-release | sed 's/"//g')
        if [[ "${option}" == "base" || "${option}" == "all" ]]; then
            _checkIsLocal
            _installUbuntuPackages "${version}"
            _installUbuntuCleanUp
        fi
        if [[ "${option}" == "common" || "${option}" == "all" ]]; then
            _installCommonDev
            if _versionCompare ${version} -gt 22.10; then
                version=22.10
            fi
            _installOrTools "ubuntu" "${version}" "amd64"
        fi
        if [[ ${CI} == "yes" ]]; then
            _installCI
        fi
        ;;
    "Red Hat Enterprise Linux")
        if [[ ${CI} == "yes" ]]; then
            echo "WARNING: Installing CI dependencies is only supported on Ubuntu 22.04" >&2
        fi
        if [[ "${option}" == "base" || "${option}" == "all" ]]; then
            _checkIsLocal
            _installRHELPackages
            _installRHELCleanUp
        fi
        if [[ "${option}" == "common" || "${option}" == "all" ]]; then
            _installCommonDev
            _installOrTools "centos" "8" "amd64"
        fi
        ;;
    "Darwin" )
        if [[ ${CI} == "yes" ]]; then
            echo "WARNING: Installing CI dependencies is only supported on Ubuntu 22.04" >&2
        fi
        _installDarwin
        cat <<EOF

To install or run openroad, update your path with:
    export PATH="\$(brew --prefix bison)/bin:\$(brew --prefix flex)/bin:\$(brew --prefix tcl-tk)/bin:\${PATH}"
    export CMAKE_PREFIX_PATH=\$(brew --prefix or-tools)
EOF
        ;;
    "openSUSE Leap" )
        if [[ ${CI} == "yes" ]]; then
            echo "WARNING: Installing CI dependencies is only supported on Ubuntu 22.04" >&2
        fi
        if [[ "${option}" == "base" || "${option}" == "all" ]]; then
            _checkIsLocal
            _installOpenSusePackages
            _installOpenSuseCleanUp
        fi
        if [[ "${option}" == "common" || "${option}" == "all" ]]; then
            _installCommonDev
            _installOrTools "opensuse" "leap" "amd64"
        fi
        cat <<EOF
To enable GCC-11 you need to run:
        update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 50
        update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 50
EOF
        ;;
    "Debian GNU/Linux" )
        version=$(awk -F= '/^VERSION_ID/{print $2}' /etc/os-release | sed 's/"//g')
        if [[ ${CI} == "yes" ]]; then
            echo "WARNING: Installing CI dependencies is only supported on Ubuntu 22.04" >&2
        fi
        if [[ "${option}" == "base" || "${option}" == "all" ]]; then
            _checkIsLocal
            _installDebianPackages "${version}"
            _installDebianCleanUp
        fi
        if [[ "${option}" == "common" || "${option}" == "all" ]]; then
            _installCommonDev
            _installOrTools "debian" "${version}" "amd64"
        fi
        ;;
    *)
        echo "unsupported system: ${os}" >&2
        _help
        ;;
esac
