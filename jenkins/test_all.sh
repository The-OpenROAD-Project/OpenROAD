# exit if any regression script fails
set -e
test/regression
src/pdngen/test/regression
src/resizer/test/regression
src/opendp/test/regression
src/TritonMacroPlace/test/regression
src/FastRoute/test/regression
src/ioPlacer/test/regression
src/TritonCTS/test/regression
src/tapcell/test/regression

# hangs indefinitely
#pushd src/replace/test && python3 regression.py run openroad && popd


