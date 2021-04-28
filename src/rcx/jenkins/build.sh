docker build --target base-dependencies -t openrcx .
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenRCX opendb bash -c "./OpenRCX/jenkins/install.sh"
