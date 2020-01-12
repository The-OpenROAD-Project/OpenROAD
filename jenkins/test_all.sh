# exit if any regression script fails
set -e
test/regression
src/pdngen/test/regression
src/resizer/test/regression
src/opendp/test/regression
src/TritonMacroPlace/test/regression

# fails every test
#src/TritonCTS/tests/run_all.sh ./build/src/openroad ./src/TritonCTS/tests

# hangs indefinitely
#pushd src/replace/test && python3 regression.py run openroad && popd

# errors: sed: 1: "/Users/cherry/sta/openr ...": command c expects \ followed by text
#cp etc/PO* . && tclsh ./src/FastRoute/tests/run_all.tcl

# errors; readlink: illegal option -- f
#src/ioPlacer/tests/run_all.sh ./build/src/openroad ./src/ioPlacer/tests/
