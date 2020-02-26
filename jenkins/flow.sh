# exit on failure of any test line
set -x
set -e
# test
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "ls"
