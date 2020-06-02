#!/bin/bash
set -x
set -e
docker build -t openroad/openroad --target base-dependencies .
echo "User is ${USER} $(id -u ${USER})"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "./OpenROAD/jenkins/install.sh"
