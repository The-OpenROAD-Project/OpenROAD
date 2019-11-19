docker build -t openroad/openroad --target base-dependencies .
docker run -v $(pwd):/OpenROAD openroad/openroad bash -c "./OpenROAD/jenkins/install.sh"
