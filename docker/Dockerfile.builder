# create image with all dependencies to compiler openroad app
# copy source code to the docker image and compile the app
# NOTE: don't use this file directly unless you know what you are doing,
# instead use etc/DockerHelper.sh
ARG fromImage=openroadeda/centos7-dev
FROM $fromImage

COPY . /OpenROAD
WORKDIR /OpenROAD

ARG compiler=gcc
RUN ./etc/Build.sh -compiler=${compiler}
