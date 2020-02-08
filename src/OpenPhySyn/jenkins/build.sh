#/bin/bash
set -x
set -e
docker build -t openroad/openphysyn --target base-dependencies .
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenPhySyn openroad/openphysyn bash -c "/OpenPhySyn/jenkins/install.sh"