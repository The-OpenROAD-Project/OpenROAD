# Must be run from outside top level directory named OpenROAD due to
# brain damage in replace and fastroute regressions.

# exit if any regression script fails
set -e
#test/regression fast
#src/pdngen/test/regression fast
#src/resizer/test/regression fast
#src/opendp/test/regression fast
pushd src/replace/test && python3 regression.py run openroad && popd
# fastroute: drop a few turds in the filesystem before testing
#tclsh src/FastRoute/tests/run_all.tcl
