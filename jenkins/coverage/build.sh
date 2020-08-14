#!/bin/bash
set -x
set -e
docker build -t openroad/openroad --target coverage .
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "./OpenROAD/jenkins/coverage/install.sh"
