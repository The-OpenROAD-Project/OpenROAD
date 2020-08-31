# exit on failure of any test line	
set -x	
set -e	
# Modify test/regression to add tests	
if [[ $# -eq 2 ]]; then
    TARGET_OS=".$1.$2"
    docker run "openroad/openroad$TARGET_OS" bash -c "/OpenROAD/test/regression"
else
    docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/test/regression"
fi
