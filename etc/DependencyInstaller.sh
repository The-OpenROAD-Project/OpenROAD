#!/usr/bin/env bash

set -euo pipefail

CMAKE_PACKAGE_ROOT_ARGS=""

_versionCompare() {
    local a b IFS=. ; set -f
    printf -v a %08d $1; printf -v b %08d $3
    test $a "$2" $b
}

_equivalenceDeps() {
    yosysVersion=v0.53

    # yosys
    yosysPrefix=${PREFIX:-"/usr/local"}
    if [[ ! $(command -v yosys) || ! $(command -v yosys-config)  ]]; then (
        cd "${baseDir}"
        git clone --depth=1 -b "${yosysVersion}" --recursive https://github.com/YosysHQ/yosys
        cd yosys
        # use of no-register flag is required for some compilers,
        # e.g., gcc and clang from RHEL8
        make -j $(nproc) PREFIX="${yosysPrefix}" ABC_ARCHFLAGS=-Wno-register
        make install
    ) fi

    # eqy
    eqyPrefix=${PREFIX:-"/usr/local"}
    if ! command -v eqy &> /dev/null; then (
        cd "${baseDir}"
        git clone --depth=1 -b "${yosysVersion}" https://github.com/YosysHQ/eqy
        cd eqy
        export PATH="${yosysPrefix}/bin:${PATH}"
        make -j $(nproc) PREFIX="${eqyPrefix}"
        make install PREFIX="${eqyPrefix}"
    )
    fi

    # sby
    sbyPrefix=${PREFIX:-"/usr/local"}
    if ! command -v sby &> /dev/null; then (
        cd "${baseDir}"
        git clone --depth=1 -b "${yosysVersion}" --recursive https://github.com/YosysHQ/sby
        cd sby
        export PATH="${eqyPrefix}/bin:${PATH}"
        make -j $(nproc) PREFIX="${sbyPrefix}" install
    )
    fi
}

_installCommonDev() {
    lastDir="$(pwd)"
    arch=$(uname -m)
    # tools versions
    osName="linux"
    if [[ "${arch}" == "aarch64" ]]; then
        cmakeChecksum="938ea8e8ecbcef24f33a7d286a00541c"
    else
        cmakeChecksum="f4d3e86abf624d73ee8dae826bbd6121"
    fi
    cmakeVersionBig=3.31
    cmakeVersionSmall=${cmakeVersionBig}.6
    pcreVersion=10.42
    pcreChecksum="37d2f77cfd411a3ddf1c64e1d72e43f7"
    swigVersion=4.1.0
    swigChecksum="794433378154eb61270a3ac127d9c5f3"
    boostVersionBig=1.86
    boostVersionSmall=${boostVersionBig}.0
    boostChecksum="ac857d73bb754b718a039830b07b9624"
    eigenVersion=3.4
    cuddVersion=3.0.0
    lemonVersion=1.3.1
    spdlogVersion=1.15.0
    gtestVersion=1.13.0
    gtestChecksum="a1279c6fb5bf7d4a5e0d0b2a4adb39ac"
    bisonVersion=3.8.2
    bisonChecksum="1e541a097cda9eca675d29dd2832921f"

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
        eval wget https://cmake.org/files/v${cmakeVersionBig}/cmake-${cmakeVersionSmall}-${osName}-${arch}.sh
        md5sum -c <(echo "${cmakeChecksum} cmake-${cmakeVersionSmall}-${osName}-${arch}.sh") || exit 1
        chmod +x cmake-${cmakeVersionSmall}-${osName}-${arch}.sh
        ./cmake-${cmakeVersionSmall}-${osName}-${arch}.sh --skip-license --prefix=${cmakePrefix}
    else
        echo "CMake already installed."
    fi

    # bison
    bisonInstalledVersion="none"
    bisonPrefix=${PREFIX:-"/usr"}
    if [ -f ${bisonPrefix}/bin/bison ]; then
        bisonInstalledVersion=$(${bisonPrefix}/bin/bison --version | awk 'NR==1 {print $NF}')
    fi
    if [ ${bisonInstalledVersion} != ${bisonVersion} ]; then
        cd "${baseDir}"
        eval wget https://ftp.gnu.org/gnu/bison/bison-${bisonVersion}.tar.gz
        md5sum -c <(echo "${bisonChecksum} bison-${bisonVersion}.tar.gz") || exit 1
        tar xf bison-${bisonVersion}.tar.gz
        cd bison-${bisonVersion}
        ./configure --prefix=${bisonPrefix}
        make -j install
        echo "bison ${bisonVersion} installed (from ${bisonInstalledVersion})."
    else
        echo "bison ${bisonVersion} already installed."
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D bison_ROOT=$(realpath ${bisonPrefix}) "

    # SWIG
    swigPrefix=${PREFIX:-"/usr/local"}
    swigBin=${swigPrefix}/bin/swig
    if [[ ! -f ${swigBin} || -z $(${swigBin} -version | grep ${swigVersion}) ]]; then
        cd "${baseDir}"
        tarName="v${swigVersion}.tar.gz"
        eval wget https://github.com/swig/swig/archive/${tarName}
        md5sum -c <(echo "${swigChecksum} ${tarName}") || exit 1
        tar xfz ${tarName}
        cd swig-${tarName%%.tar*} || cd swig-${swigVersion}

        # Check if pcre2 is installed
        if [[ -z $(pcre2-config --version) ]]; then
            tarName="pcre2-${pcreVersion}.tar.gz"
            eval wget https://github.com/PCRE2Project/pcre2/releases/download/pcre2-${pcreVersion}/${tarName}
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
    CMAKE_PACKAGE_ROOT_ARGS+=" -D SWIG_ROOT=$(realpath $swigPrefix) "

    # boost
    boostPrefix=${PREFIX:-"/usr/local"}
    if [[ -z $(grep "BOOST_LIB_VERSION \"${boostVersionBig//./_}\"" ${boostPrefix}/include/boost/version.hpp 2> /dev/null) ]]; then
        cd "${baseDir}"
        boostVersionUnderscore=${boostVersionSmall//./_}
        eval wget https://archives.boost.io/release/${boostVersionSmall}/source/boost_${boostVersionUnderscore}.tar.gz
        md5sum -c <(echo "${boostChecksum}  boost_${boostVersionUnderscore}.tar.gz") || exit 1
        tar -xf boost_${boostVersionUnderscore}.tar.gz
        cd boost_${boostVersionUnderscore}
        ./bootstrap.sh --prefix="${boostPrefix}"
        ./b2 install --with-iostreams --with-test --with-serialization --with-system --with-thread -j $(nproc)
    else
        echo "Boost already installed."
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D Boost_ROOT=$(realpath $boostPrefix) "

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
    CMAKE_PACKAGE_ROOT_ARGS+=" -D Eigen3_ROOT=$(realpath $eigenPrefix) "

    # cudd
    cuddPrefix=${PREFIX:-"/usr/local"}
    if [[ ! -f ${cuddPrefix}/include/cudd.h ]]; then
        cd "${baseDir}"
        git clone --depth=1 -b ${cuddVersion} https://github.com/The-OpenROAD-Project/cudd.git
        cd cudd
        autoreconf
        ./configure --prefix=${cuddPrefix}
        make -j $(nproc) install
    else
        echo "Cudd already installed."
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D cudd_ROOT=$(realpath $cuddPrefix) "

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
    CMAKE_PACKAGE_ROOT_ARGS+=" -D cusp_ROOT=$(realpath $cuspPrefix) "

    # lemon
    lemonPrefix=${PREFIX:-"/usr/local"}
    if [[ -z $(grep "LEMON_VERSION \"${lemonVersion}\"" ${lemonPrefix}/include/lemon/config.h 2> /dev/null) ]]; then
        cd "${baseDir}"
        git clone --depth=1 -b ${lemonVersion} https://github.com/The-OpenROAD-Project/lemon-graph.git
        cd lemon-graph
        ${cmakePrefix}/bin/cmake -DCMAKE_INSTALL_PREFIX="${lemonPrefix}" -B build .
        ${cmakePrefix}/bin/cmake --build build -j $(nproc) --target install
    else
        echo "Lemon already installed."
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D LEMON_ROOT=$(realpath $lemonPrefix) "

    # spdlog
    spdlogPrefix=${PREFIX:-"/usr/local"}
    spdlogInstalledVersion="none"
    if [ -d ${spdlogPrefix}/include/spdlog ]; then
        spdlogInstalledVersion=$(grep "#define SPDLOG_VER_" ${spdlogPrefix}/include/spdlog/version.h | sed 's/.*\s//' | tr '\n' '.' | sed 's/\.$//')
    fi
    if [ ${spdlogInstalledVersion} != ${spdlogVersion} ]; then
        cd "${baseDir}"
        git clone --depth=1 -b "v${spdlogVersion}" https://github.com/gabime/spdlog.git
        cd spdlog
        ${cmakePrefix}/bin/cmake -DCMAKE_INSTALL_PREFIX="${spdlogPrefix}" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DSPDLOG_BUILD_EXAMPLE=OFF -B build .
        ${cmakePrefix}/bin/cmake --build build -j $(nproc) --target install
        echo "spdlog ${spdlogVersion} installed (from ${spdlogInstalledVersion})."
    else
        echo "spdlog ${spdlogVersion} already installed."
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D spdlog_ROOT=$(realpath $spdlogPrefix) "

    # gtest
    gtestPrefix=${PREFIX:-"/usr/local"}
    if [[ ! -d ${gtestPrefix}/include/gtest ]]; then
        cd "${baseDir}"
        eval wget https://github.com/google/googletest/archive/refs/tags/v${gtestVersion}.zip
        md5sum -c <(echo "${gtestChecksum} v${gtestVersion}.zip") || exit 1
        unzip v${gtestVersion}.zip
        cd googletest-${gtestVersion}
        ${cmakePrefix}/bin/cmake -DCMAKE_INSTALL_PREFIX="${gtestPrefix}" -B build .
        ${cmakePrefix}/bin/cmake --build build --target install
    else
        echo "gtest already installed."
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D GTest_ROOT=$(realpath $gtestPrefix) "

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
            eval wget -O ninja-linux.zip https://github.com/ninja-build/ninja/releases/download/v${ninjaVersion}/ninja-linux.zip
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
    osVersion=$2
    arch=$3
    orToolsVersionBig=9.11
    orToolsVersionSmall=${orToolsVersionBig}.4210

    rm -rf "${baseDir}"
    mkdir -p "${baseDir}"
    if [[ ! -z "${PREFIX}" ]]; then mkdir -p "${PREFIX}"; fi
    cd "${baseDir}"

    # Disable exit on error for 'find' command, as it might return non zero
    set +euo pipefail
    LIST=($(find /local* /opt* /lib* /usr* /bin* -type f -name "libortools.so*" 2>/dev/null))
    # Bring back exit on error
    set -euo pipefail
    # Return if right version of or-tools is installed
    for lib in ${LIST[@]}; do
        if [[ "$lib" =~ .*"/libortools.so.${orToolsVersionSmall}" ]]; then
            echo "OR-Tools is already installed"
            CMAKE_PACKAGE_ROOT_ARGS+=" -D ortools_ROOT=$(realpath $(dirname $lib)/..) "
            return
        fi
    done

    orToolsPath=${PREFIX:-"/opt/or-tools"}
    if [ "$(uname -m)" == "aarch64" ]; then
        echo "OR-TOOLS NOT FOUND"
        echo "Installing  OR-Tools for aarch64..."
        git clone --depth=1 -b "v${orToolsVersionBig}" https://github.com/google/or-tools.git
        cd or-tools
        ${cmakePrefix}/bin/cmake -S. -Bbuild -DBUILD_DEPS:BOOL=ON -DBUILD_EXAMPLES:BOOL=OFF -DBUILD_SAMPLES:BOOL=OFF -DBUILD_TESTING:BOOL=OFF -DCMAKE_INSTALL_PREFIX=${orToolsPath} -DCMAKE_CXX_FLAGS="-w" -DCMAKE_C_FLAGS="-w"
        ${cmakePrefix}/bin/cmake --build build --config Release --target install -v -j $(nproc)
    else
        if [[ $osVersion == rodete ]]; then
            osVersion=11
        fi
        if [[ $os == ubuntu && $osVersion == 25.04 ]]; then
            # FIXME make do with or-tools for 24.04 until an official release for 25.04 is available
            osVersion=24.04
        fi
        orToolsFile=or-tools_${arch}_${os}-${osVersion}_cpp_v${orToolsVersionSmall}.tar.gz
        eval wget https://github.com/google/or-tools/releases/download/v${orToolsVersionBig}/${orToolsFile}
        if command -v brew &> /dev/null; then
            orToolsPath="$(brew --prefix or-tools)"
        fi
        mkdir -p ${orToolsPath}
        tar --strip 1 --dir ${orToolsPath} -xf ${orToolsFile}
        rm -rf ${baseDir}
    fi
    CMAKE_PACKAGE_ROOT_ARGS+=" -D ortools_ROOT=$(realpath $orToolsPath) "
}

_installUbuntuCleanUp() {
    apt-get autoclean -y
    apt-get autoremove -y
}

_installUbuntuPackages() {
    export DEBIAN_FRONTEND="noninteractive"
    apt-get -y update
    apt-get -y install --no-install-recommends tzdata
    apt-get -y install --no-install-recommends \
        automake \
        autotools-dev \
        binutils \
        bison \
        build-essential \
        ccache \
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
        libfl-dev \
        libgomp1 \
        libomp-dev \
        libpcre2-dev \
        libpcre3-dev \
        libreadline-dev \
        pandoc \
        python3-dev \
        qt5-image-formats-plugins \
        tcl \
        tcl-dev \
        tcl-tclreadline \
        tcllib \
        unzip \
        wget \
        zlib1g-dev

    packages=()
    # Chose Python version
    if _versionCompare $1 -ge 25.04; then
        packages+=("libtcl8.6")
    else
        packages+=("libtcl")
    fi
    # Chose Python version
    if _versionCompare $1 -ge 25.04; then
        packages+=("libpython3.13")
    elif _versionCompare $1 -ge 24.04; then
        packages+=("libpython3.12")
    elif _versionCompare $1 -ge 22.10; then
        packages+=("libpython3.11")
    else
        packages+=("libpython3.8")
    fi

    # Chose QT libraries
    if _versionCompare $1 -ge 22.04; then
        packages+=(
            "qt5-qmake" \
            "qtbase5-dev" \
            "qtbase5-dev-tools" \
            "libqt5charts5-dev" \
            "qtchooser" \
        )
    else
        packages+=(
            "libqt5charts5-dev" \
            "qt5-default" \
        )
    fi
    apt-get install -y --no-install-recommends ${packages[@]}
}

_installRHELCleanUp() {
    yum clean -y all
    rm -rf /var/lib/apt/lists/*
}

_installRHELPackages() {
    arch=amd64
    pandocVersion=3.1.11.1

    yum -y update
    yum -y install tzdata
    yum -y install redhat-rpm-config rpm-build
    yum -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm
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
        libffi-devel \
        libtool \
        llvm \
        llvm-devel \
        llvm-libs \
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
        qt5-qtcharts-devel \
        qt5-qtimageformats \
        readline \
        tcl-tclreadline \
        tcl-tclreadline-devel \
        tcl-thread-devel \
        tcllib \
        wget \
        zlib-devel

    yum install -y \
        https://mirror.stream.centos.org/9-stream/AppStream/x86_64/os/Packages/flex-2.6.4-9.el9.x86_64.rpm \
        https://mirror.stream.centos.org/9-stream/AppStream/x86_64/os/Packages/readline-devel-8.1-4.el9.x86_64.rpm \
        https://rpmfind.net/linux/centos-stream/9-stream/AppStream/x86_64/os/Packages/tcl-devel-8.6.10-7.el9.x86_64.rpm

    eval wget https://github.com/jgm/pandoc/releases/download/${pandocVersion}/pandoc-${pandocVersion}-linux-${arch}.tar.gz
    tar xvzf pandoc-${pandocVersion}-linux-${arch}.tar.gz --strip-components 1 -C /usr/local/
    rm -rf pandoc-${pandocVersion}-linux-${arch}.tar.gz
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
    brew install bison boost cmake eigen flex fmt groff libomp or-tools pandoc pyqt5 python spdlog tcl-tk zlib

    # Some systems need this to correctly find OpenMP package during build
    brew link --force libomp

    # Lemon is not in the homebrew-core repo
    brew install The-OpenROAD-Project/lemon-graph/lemon-graph

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
    apt-get -y install --no-install-recommends tzdata
    if [[ $1 == rodete ]]; then
        tclver=8.6
    else
        tclver=
    fi
    apt-get -y install --no-install-recommends \
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
        libfl-dev \
        libgomp1 \
        libomp-dev \
        libpcre2-dev \
        libpcre3-dev \
        libreadline-dev \
        libtcl${tclver} \
        pandoc \
        python3-dev \
        qt5-image-formats-plugins \
        tcl-dev \
        tcl-tclreadline \
        tcllib \
        unzip \
        wget \
        zlib1g-dev

    if [[ $1 == 10 ]]; then
        apt-get install -y --no-install-recommends \
            libpython3.7 \
            libqt5charts5-dev \
            qt5-default

    else
        if [[ $1 == rodete ]]; then
            pythonver=3.12
        else
            pythonver=3.8
        fi
        apt-get install -y --no-install-recommends \
            libpython${pythonver} \
            libqt5charts5-dev \
            qtbase5-dev \
            qtchooser \
            qt5-qmake \
            qtbase5-dev-tools
    fi
}

_installCI() {
    apt-get -y update
    apt-get -y install --no-install-recommends \
        apt-transport-https \
        ca-certificates \
        curl \
        default-jdk \
        gnupg \
        python3 \
        python3-pip \
        python3-pandas \
        jq \
        lsb-release \
        parallel \
        software-properties-common \
        time \
        unzip zip

    curl -Lo bazelisk https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64
    chmod +x bazelisk
    mv bazelisk /usr/local/bin/bazel

    if command -v docker &> /dev/null; then
        # The user can uninstall docker if they want to reinstall it,
        # and also this allows the user to choose drop in replacements
        # for docker, such as podman-docker
        echo "Docker is already installed, skip docker reinstall."
        return 0
    fi

    # Add Docker's official GPG key:
    install -m 0755 -d /etc/apt/keyrings
    curl -fsSL https://download.docker.com/linux/ubuntu/gpg \
        -o /etc/apt/keyrings/docker.asc

    chmod a+r /etc/apt/keyrings/docker.asc

    # Add the repository to Apt sources:
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
        $(. /etc/os-release && echo "${VERSION_CODENAME}") stable" | \
        tee /etc/apt/sources.list.d/docker.list > /dev/null

    apt-get -y update
    apt-get -y install --no-install-recommends \
        docker-ce \
        docker-ce-cli \
        containerd.io \
        docker-buildx-plugin

    if _versionCompare ${1} -lt 24.04; then
        # Install clang for C++20 support
        wget https://apt.llvm.org/llvm.sh
        chmod +x llvm.sh
        ./llvm.sh 16 all
    fi

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

Usage: $0 -all
                                # Installs all of OpenROAD's dependencies no
                                #     need to run -base or -common. Requires
                                #     privileged access.
       $0 -base
                                # Installs OpenROAD's dependencies using
                                #     package managers (-common must be
                                #     executed in another command). Requires
                                #     privileged access.
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
                                #    "${HOME}/.local". Only used with
                                #    -common. This flag cannot be used with
                                #    sudo or with root access.
       $0 -ci
                                # Installs dependencies required to run CI
       $0 -nocert
                                # Disable certificate checks
                                #    WARNING: Do not use without a good reason,
                                #    like working around a firewall. This opens
                                #    vulnerability to man-in-the-middle (MITM)
                                #    attacks.
       $0 -save-deps-prefixes=FILE
                                # Dumps OpenROAD build arguments and variables
                                # to FILE
       $0 -constant-build-dir
                                # Use constant build directory, instead of
                                #    random one.

EOF
    exit "${1:-1}"
}

# Default values
PREFIX=""
option="none"
isLocal="false"
equivalenceDeps="no"
CI="no"
saveDepsPrefixes=""
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
        -all)
            if [[ "${option}" != "none" ]]; then
                echo "WARNING: previous argument -${option} will be overwritten with -all." >&2
            fi
            option="all"
            ;;
        -base)
            if [[ "${option}" != "none" ]]; then
                echo "WARNING: previous argument -${option} will be overwritten with -base." >&2
            fi
            option="base"
            ;;
        -common)
            if [[ "${option}" != "none" ]]; then
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
        -constant-build-dir)
            if [[ -d "$baseDir" ]]; then
                echo "INFO: removing old building directory $baseDir"
                rm -r "$baseDir"
            fi
            baseDir="/tmp/DependencyInstaller-OpenROAD"
            mkdir -p "$baseDir"
            ;;
        -prefix=*)
            if [[ ! -z ${PREFIX} ]]; then
                echo "WARNING: previous argument -local will be overwritten with -prefix"
                export isLocal="false"
            fi
            export PREFIX="$(realpath $(echo $1 | sed -e 's/^[^=]*=//g'))"
            ;;
        -nocert)
            echo "WARNING: security certificates for downloaded packages will not be checked. Do not use" >&2
            echo "         -nocert without a good reason, like working around a firewall. This opens" >&2
            echo "         vulnerability to man-in-the-middle (MITM) attacks." >&2
            shopt -s expand_aliases
            alias wget="wget --no-check-certificate"
            export GIT_SSL_NO_VERIFY=true
            ;;
        -save-deps-prefixes=*)
            saveDepsPrefixes=$(realpath ${1#-save-deps-prefixes=})
            ;;
        *)
            echo "unknown option: ${1}" >&2
            _help
            ;;
    esac
    shift 1
done

if [[ "${option}" == "none" ]]; then
    echo "You must use one of: -all|-base|-common" >&2
    _help
fi

if [[ -z "${saveDepsPrefixes}" ]]; then
    DIR="$(dirname $(readlink -f $0))"
    saveDepsPrefixes="$DIR/openroad_deps_prefixes.txt"
fi

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
    "Ubuntu" )
        ubuntuVersion=$(awk -F= '/^VERSION_ID/{print $2}' /etc/os-release | sed 's/"//g')
        if [[ ${CI} == "yes" ]]; then
            _installCI "${ubuntuVersion}"
        fi
        if [[ "${option}" == "base" || "${option}" == "all" ]]; then
            _checkIsLocal
            _installUbuntuPackages "${ubuntuVersion}"
            _installUbuntuCleanUp
        fi
        if [[ "${option}" == "common" || "${option}" == "all" ]]; then
            _installCommonDev
            # set version for non LTS
            if _versionCompare ${ubuntuVersion} -ge 25.04; then
                ubuntuVersion=25.04
            elif _versionCompare ${ubuntuVersion} -ge 24.04; then
                ubuntuVersion=24.04
            elif _versionCompare ${ubuntuVersion} -ge 22.04; then
                ubuntuVersion=22.04
            else
                ubuntuVersion=20.04
            fi
            _installOrTools "ubuntu" "${ubuntuVersion}" "amd64"
        fi
        ;;
    "Red Hat Enterprise Linux" | "Rocky Linux")
    if [[ "${os}" == "Red Hat Enterprise Linux" ]]; then
        rhelVersion=$(rpm -q --queryformat '%{VERSION}' redhat-release | cut -d. -f1)
    elif  [[ "${os}" == "Rocky Linux" ]]; then
        rhelVersion=$(rpm -q --queryformat '%{VERSION}' rocky-release | cut -d. -f1)
    fi
        if [[ "${rhelVersion}" != "9" ]]; then
            echo "ERROR: Unsupported ${rhelVersion} version. Only '9' is supported."
            exit 1
        fi
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
            _installOrTools "rockylinux" "9" "amd64"
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
    "Debian GNU/Linux" | "Debian GNU/Linux rodete" )
        debianVersion=$(awk -F= '/^VERSION_ID/{print $2}' /etc/os-release | sed 's/"//g')
        if [[ -z ${debianVersion} ]]; then
            debianVersion=$(awk -F= '/^VERSION_CODENAME/{print $2}' /etc/os-release | sed 's/"//g')
        fi
        if [[ ${CI} == "yes" ]]; then
            echo "WARNING: Installing CI dependencies is only supported on Ubuntu 22.04" >&2
        fi
        if [[ "${option}" == "base" || "${option}" == "all" ]]; then
            _checkIsLocal
            _installDebianPackages "${debianVersion}"
            _installDebianCleanUp
        fi
        if [[ "${option}" == "common" || "${option}" == "all" ]]; then
            _installCommonDev
            _installOrTools "debian" "${debianVersion}" "amd64"
        fi
        ;;
    *)
        echo "unsupported system: ${os}" >&2
        _help
        ;;
esac
if [[ ! -z ${saveDepsPrefixes} ]]; then
    mkdir -p "$(dirname $saveDepsPrefixes)"
    echo "$CMAKE_PACKAGE_ROOT_ARGS" > $saveDepsPrefixes
fi
