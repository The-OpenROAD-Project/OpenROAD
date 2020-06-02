#!/bin/bash
set -x
set -e
docker build -t openroad/openroad --target base-dependencies .
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "./OpenROAD/jenkins/install.sh"
