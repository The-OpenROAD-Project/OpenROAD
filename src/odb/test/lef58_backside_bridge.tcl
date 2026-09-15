# Verify that the LEF58_BACKSIDE_BRIDGE macro property is parsed and
# exposed via dbMaster::isBacksideBridge().
source "helpers.tcl"

read_lef lef58_backside_bridge.lef

set db [ord::get_db]
set libs [$db getLibs]
set lib [lindex $libs 0]
foreach master [$lib getMasters] {
  puts "[$master getName] isBacksideBridge=[$master isBacksideBridge]"
}
