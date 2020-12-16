set -e

BASE_DIR=$(dirname $0)
APP=$BASE_DIR/../build/src/swig/tcl/opendbtcl

echo "Running tests.."
echo "================="

echo "[1] Importing opendbtcl package"
$APP $BASE_DIR/tcl/01-import_package_test.tcl
echo "SUCCESS!"
echo ""

echo "[2] Reading lef files"
$APP $BASE_DIR/tcl/02-read_lef_test.tcl
echo "SUCCESS!"
echo ""

echo "[3] Dump via rules"
$APP $BASE_DIR/tcl/03-dump_via_rules_test.tcl
echo "SUCCESS!"
echo ""

echo "[4] Dump vias"
$APP $BASE_DIR/tcl/04-dump_vias_test.tcl
echo "SUCCESS!"
echo ""

echo "[5] Read DEF"
$APP $BASE_DIR/tcl/05-read_def_test.tcl
echo "SUCCESS!"
echo ""

echo "[7] Dump nets"
$APP $BASE_DIR/tcl/07-dump_nets_test.tcl
echo "SUCCESS!"
echo ""

echo "[8] Write LEF/DEF file"
$APP $BASE_DIR/tcl/08-write_lef_and_def_test.tcl
echo "SUCCESS!"
echo ""

echo "[9] Database LEF access tests"
$APP $BASE_DIR/tcl/09-lef_data_access_test.tcl
echo "SUCCESS!"
echo ""

echo "[10] Database DEF access tests (1)"
$APP $BASE_DIR/tcl/10-gcd_def_access_test.tcl
echo "SUCCESS!"
echo ""

echo "[11] Database DEF access tests (2)"
$APP $BASE_DIR/tcl/11-gcd_pdn_def_access_test.tcl
echo "SUCCESS!"
echo ""

echo "[12] Database edit DEF tests"
$APP $BASE_DIR/tcl/12-edit_def_test.tcl
echo "SUCCESS!"
echo ""

echo "[13] Wire encoder test"
$APP $BASE_DIR/tcl/13-wire_encoder_test.tcl
echo "SUCCESS!"
echo ""

echo "[14] Edit via params test"
$APP $BASE_DIR/tcl/14-edit_via_params_test.tcl
echo "SUCCESS!"
echo ""

echo "[15] Database row create test"
$APP $BASE_DIR/tcl/15-row_settings_test.tcl
echo "SUCCESS!"
echo ""

echo "[17] Database read/write test"
$APP $BASE_DIR/tcl/17-db_read-write_test.tcl
echo "SUCCESS!"
echo ""

echo "[18] Check routing tracks test"
$APP $BASE_DIR/tcl/18-check_routing_tracks.tcl
echo "SUCCESS!"
echo ""

echo "[19] Polygon operations test"
$APP $BASE_DIR/tcl/19-polygon_test.tcl
echo "SUCCESS!"
echo ""

echo "[20] DEF parser test"
$APP $BASE_DIR/tcl/20-def_parser_test.tcl
echo "SUCCESS!"
echo ""
