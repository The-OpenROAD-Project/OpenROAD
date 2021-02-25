set -e

BASE_DIR=$(dirname $0)

export PYTHONPATH=$BASE_DIR/../build/src/swig/python:$PYTHONPATH

echo "Running tests .."
echo "================"

echo "[1] Importing opendbpy package"
python3 $BASE_DIR/python/01-import_package_test.py
echo "SUCCESS!"
echo ""

echo "[2] Reading lef files"
python3 $BASE_DIR/python/02-read_lef_test.py
echo "SUCCESS!"
echo ""


echo "[3] Dump via rules"
python3 $BASE_DIR/python/03-dump_via_rules_test.py
echo "SUCCESS!"
echo ""

echo "[4] Dump vias"
python3 $BASE_DIR/python/04-dump_vias_test.py
echo "SUCCESS!"
echo ""

echo "[5] Read DEF"
python3 $BASE_DIR/python/05-read_def_test.py
echo "SUCCESS!"
echo ""

echo "[7] Dump nets"
python3 $BASE_DIR/python/07-dump_nets_test.py
echo "SUCCESS!"
echo ""

echo "[8] Write LEF/DEF file"
python3 $BASE_DIR/python/08-write_lef_and_def_test.py
echo "SUCCESS!"
echo ""

echo "[9] Database LEF access tests"
python3 $BASE_DIR/python/09-lef_data_access_test.py
echo "SUCCESS!"
echo ""

echo "[10] Database DEF access tests (1)"
python3 $BASE_DIR/python/10-gcd_def_access_test.py
echo "SUCCESS!"
echo ""

echo "[11] Database DEF access tests (2)"
python3 $BASE_DIR/python/11-gcd_pdn_def_access_test.py
echo "SUCCESS!"
echo ""

echo "[12] Database edit DEF tests"
python3 $BASE_DIR/python/12-edit_def_test.py
echo "SUCCESS!"
echo ""

echo "[13] Wire encoder test"
python3 $BASE_DIR/python/13-wire_encoder_test.py
echo "SUCCESS!"
echo ""

echo "[14] Edit via params test"
python3 $BASE_DIR/python/14-edit_via_params_test.py
echo "SUCCESS!"
echo ""

echo "[15] Database row create test"
python3 $BASE_DIR/python/15-row_settings_test.py
echo "SUCCESS!"
echo ""

echo "[16] Database def octilinear read write test"
python3 $BASE_DIR/python/16-db-read-write-octilinear-def_test.py
echo "SUCCESS!"
echo ""

echo "[17] Database read/write test"
python3 $BASE_DIR/python/17-db_read-write_test.py
echo "SUCCESS!"
echo ""


echo "[18] Multiple Boxes per dbBPin test"
python3 $BASE_DIR/python/18-multiple_boxes_pin_test.py
echo "SUCCESS!"
echo ""