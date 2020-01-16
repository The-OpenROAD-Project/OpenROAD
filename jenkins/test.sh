set -x
set -e
# exit on failure of any test line
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/test/regression"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/src/init_fp/test/regression"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/src/pdngen/test/regression"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/src/resizer/test/regression"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/src/opendp/test/regression"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/src/TritonMacroPlace/test/regression"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "cd /OpenROAD/src/replace/test && python3 regression.py run openroad"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/src/FastRoute/test/regression"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/src/ioPlacer/test/regression"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/src/TritonCTS/test/regression"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/src/tapcell/test/regression"
