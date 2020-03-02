# exit on failure of any test line
set -x
set -e
# Modify test/regression to add tests
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/test/regression"
# one bad actor
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "cd /OpenROAD/src/replace/test && python3 regression.py run openroad"

