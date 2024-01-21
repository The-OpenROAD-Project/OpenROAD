#!/bin/bash
#
# deltaDebug.py smoke-test, run from ORFS/flow folder
# exit with error if anything is amiss, including evaluation of
# variable names such as $(false), unused variables, etc.
set -ue -o pipefail

testname=uart

make DESIGN_CONFIG=designs/asap7/$testname/config.mk place
make DESIGN_CONFIG=designs/asap7/$testname/config.mk global_place_issue
latest_file=$(ls -t global_place_${testname}_asap7_base*.tar.gz | head -n1)
echo "Testing $latest_file"
. ../env.sh
rm -rf test_delta_debug
mkdir test_delta_debug
cd test_delta_debug
tar --strip-components=1 -xzf ../$latest_file
sed -i 's/openroad -no_init/openroad -exit -no_init/g' run-me-$testname-asap7-base.sh
openroad -exit -python ~/OpenROAD-flow-scripts/tools/OpenROAD/etc/deltaDebug.py --persistence 3 --use_stdout --error_string "Iter: 100 " --base_db_path results/asap7/$testname/base/3_2_place_iop.odb --step ./run-me-$testname-asap7-base.sh --multiplier 2
