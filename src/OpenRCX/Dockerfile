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

# Install epel repo
RUN wget https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
RUN yum install -y epel-release-latest-7.noarch.rpm

# Install dev and runtime dependencies
RUN yum install -y tcl-devel qt3-devel itcl-devel tcl tk ksh libstdc++ qt3 \
    itcl iwidgets blt tcllib bwidget boost-devel

# Install python dev
RUN yum install -y https://centos7.iuscommunity.org/ius-release.rpm && \
    yum update -y && \
    yum install -y python36u python36u-libs python36u-devel python36u-pip

RUN mkdir /.local && chmod 777 /.local && chmod 777 /usr/lib/python3.6/site-packages/

FROM base-dependencies AS builder


COPY . /OpenDB
WORKDIR /OpenDB

# Build
RUN mkdir build
RUN cd build && cmake .. && make
