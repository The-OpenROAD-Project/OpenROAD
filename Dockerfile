FROM centos:centos7 AS base-dependencies
LABEL maintainer "Abdelrahman Hosny <abdelrahman_hosny@brown.edu>"

# Install Development Environment
RUN yum group install -y "Development Tools"
RUN yum install -y wget git
RUN yum -y install centos-release-scl && \
    yum -y install devtoolset-8 devtoolset-8-libatomic-devel
RUN wget https://cmake.org/files/v3.14/cmake-3.14.0-Linux-x86_64.sh && \
    chmod +x cmake-3.14.0-Linux-x86_64.sh  && \
    ./cmake-3.14.0-Linux-x86_64.sh --skip-license --prefix=/usr/local

# Install gcc 7.3
WORKDIR /tmp
RUN curl -O https://ftp.gnu.org/gnu/gcc/gcc-7.3.0/gcc-7.3.0.tar.gz
RUN tar xzf gcc-7.3.0.tar.gz
WORKDIR /tmp/gcc-7.3.0
RUN ./contrib/download_prerequisites
RUN cd ..
RUN mkdir gcc-build
RUN cd gcc-build
RUN ../gcc-7.3.0/configure                           \
    --prefix=/usr                                    \
    --enable-shared                                  \
    --enable-threads=posix                           \
    --enable-__cxa_atexit                            \
    --enable-clocale=gnu                             \
    --disable-multilib                               \
    --enable-languages=all
RUN make -j$(nproc)
RUN make install
WORKDIR /tmp

# Install CMake
RUN wget https://cmake.org/files/v3.14/cmake-3.14.0-Linux-x86_64.sh && \
    chmod +x cmake-3.14.0-Linux-x86_64.sh  && \
    ./cmake-3.14.0-Linux-x86_64.sh --skip-license --prefix=/usr/local

# Install epel repo
RUN wget https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
RUN yum install -y epel-release-latest-7.noarch.rpm


# Install dev and runtime dependencies
RUN yum install -y tcl-devel tcl tk libstdc++ tk-devel pcre-devel

# Install Boost
RUN wget http://downloads.sourceforge.net/project/boost/boost/1.67.0/boost_1_67_0.tar.gz \
    && tar xfz boost_1_67_0.tar.gz \
    && rm -f boost_1_67_0.tar.gz \
    && cd boost_1_67_0 \
    && ./bootstrap.sh --prefix=/usr --with-libraries=program_options,log,filesystem \
    && ./b2 cxxflags=-fPIC install -j $(nproc) \
    && cd /tmp \
    && rm -rf boost_1_67_0

# Install SWIG
RUN yum remove -y swig \
    && wget https://github.com/swig/swig/archive/rel-3.0.12.tar.gz \
    && tar xfz rel-3.0.12.tar.gz \
    && rm -rf rel-3.0.12.tar.gz \
    && cd swig-rel-3.0.12 \
    && ./autogen.sh && ./configure --prefix=/usr && make -j $(nproc) && make install \
    && cd /tmp \
    && rm -rf swig-rel-3.0.12


# Install python dev
RUN yum install -y https://centos7.iuscommunity.org/ius-release.rpm && \
    yum update -y && \
    yum install -y python36u python36u-libs python36u-devel python36u-pip

WORKDIR /OpenROAD

FROM base-dependencies AS builder

COPY . /OpenROAD
WORKDIR /OpenROAD

# Build
RUN mkdir build
RUN cd build && cmake .. && make -j 4
