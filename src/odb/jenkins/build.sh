docker build --target base-dependencies -t opendb .
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenDB opendb bash -c "./OpenDB/jenkins/install.sh"