# create image with all dependencies to compiler openroad app
# copy source code to the docker image and compile the app
# NOTE: don't use this file directly unless you know what you are doing,
# instead use etc/DockerHelper.sh

# https://github.com/moby/moby/issues/38379#issuecomment-448445652
ARG fromImage=openroad/centos7-dev:latest

FROM $fromImage

ARG compiler=gcc
ARG numThreads=$(nproc)

COPY . /OpenROAD
WORKDIR /OpenROAD

RUN ./etc/Build.sh -compiler=${compiler} -threads=${numThreads}
