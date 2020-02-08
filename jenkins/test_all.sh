# exit if any regression script fails
set -e
test/regression
src/init_fp/test/regression
src/ioPlacer/test/regression
src/pdngen/test/regression
src/TritonMacroPlace/test/regression
src/replace/test/regression
src/resizer/test/regression
src/TritonCTS/test/regression
src/opendp/test/regression
src/FastRoute/test/regression
src/tapcell/test/regression
