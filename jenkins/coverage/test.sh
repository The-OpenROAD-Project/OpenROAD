# exit on failure of any test line
set -x
set -e
# Modify test/regression to add tests
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/test/regression"
docker run -v $(pwd):/OpenROAD openroad/openroad bash -c "yum install -y http://downloads.sourceforge.net/ltp/lcov-1.14-1.noarch.rpm && lcov --capture --directory /OpenROAD/build --exclude '/usr/include/*' --exclude '/opt/*' --exclude '/usr/lib/*' --exclude '/usr/local/*' --exclude '*build*' --output-file /OpenROAD/main_coverage.info && genhtml /OpenROAD/main_coverage.info --output-directory /OpenROAD/out --ignore-errors source"
