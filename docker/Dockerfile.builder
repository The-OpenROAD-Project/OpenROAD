# create image with all dependencies to compiler openroad app
# copy source code to the docker image and compile the app
# NOTE: don't use this file directly unless you know what you are doing,
# instead use etc/DockerHelper.sh

# https://github.com/moby/moby/issues/38379#issuecomment-448445652
ARG fromImage=openroad/ubuntu22.04-dev:latest

FROM $fromImage

ARG compiler=gcc
ARG numThreads=$(nproc)
ARG depsPrefixFile="/etc/openroad_deps_prefixes.txt"
ARG LOCAL_PATH=""

COPY . /OpenROAD
WORKDIR /OpenROAD

ENV PATH=${LOCAL_PATH}:${PATH}

RUN apt-get update && apt-get install -y \
    libcurl4-openssl-dev \
    wget && \
    wget https://boostorg.jfrog.io/artifactory/main/release/1.80.0/source/boost_1_80_0.tar.gz && \
    tar -xzvf boost_1_80_0.tar.gz && \
    cd boost_1_80_0 && \
    ./bootstrap.sh --with-libraries=json && \
    ./b2 install -j$(nproc) && \
    cd .. && rm -rf boost_1_80_0 boost_1_80_0.tar.gz

RUN apt-get update && \
apt-get install -y \
libcurl4-openssl-dev \
libboost-all-dev

RUN ./etc/Build.sh -compiler=${compiler} -threads=${numThreads} -deps-prefixes-file=${depsPrefixFile}
