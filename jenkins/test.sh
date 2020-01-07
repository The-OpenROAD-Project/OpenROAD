set -x
set -e
# exit on failure of any test line
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/test/regression fast"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/src/pdngen/test/regression fast"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/src/resizer/test/regression fast"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/src/opendp/test/regression fast"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/src/TritonMacroPlace/test/regression fast"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "cd /OpenROAD/src/replace/test && python3 regression.py run openroad"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "cd /OpenROAD && cp etc/PO* . && tclsh ./src/FastRoute/tests/run_all.tcl"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "cd /OpenROAD && ./src/ioPlacer/tests/run_all.sh ./build/src/openroad ./src/ioPlacer/tests/"
docker run -u $(id -u ${USER}):$(id -g ${USER}) -v $(pwd):/OpenROAD openroad/openroad bash -c "cd /OpenROAD && ./src/TritonCTS/tests/run_all.sh ./build/src/openroad ./src/TritonCTS/tests/"
docker run -v $(pwd):/OpenROAD openroad/openroad bash -c "cd /OpenROAD && tclsh ./src/tapcell/test/run_all.tcl"
