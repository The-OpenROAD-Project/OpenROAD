# exit if any regression script fails
set -e
test/regression
src/pdngen/test/regression
src/resizer/test/regression
src/opendp/test/regression
src/TritonMacroPlace/test/regression

# these still use the wrong directory and script name
src/TritonCTS/tests/run_all.sh
src/ioPlacer/tests/run_all.sh

# hangs indefinitely
#pushd src/replace/test && python3 regression.py run openroad && popd

# errors: sed: 1: "/Users/cherry/sta/openr ...": command c expects \ followed by text
#cp etc/PO* . && tclsh ./src/FastRoute/tests/run_all.tcl


