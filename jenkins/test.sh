# Modify test/regression to add tests
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/test/regression"
