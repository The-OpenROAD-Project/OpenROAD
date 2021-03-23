# create image with dependencies, copy source code and compile the app
ARG fromImage=openroadeda/centos7-base
ARG compiler=gcc
FROM $fromImage
COPY . /OpenROAD
WORKDIR /OpenROAD
RUN ./etc/BuildHelper $compiler
