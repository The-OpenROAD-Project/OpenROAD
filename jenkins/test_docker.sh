# exit on failure of any test line	
set -x	
set -e	
# Modify test/regression to add tests	
if [[ $# -eq 2 ]]; then
    TARGET_OS="$1"
    TARGET_COMPILER="$2"
    docker run "openroad/openroad_${TARGET_OS}_${TARGET_COMPILER}" bash -c "/OpenROAD/test/regression"
else
    docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/test/regression"
fi
