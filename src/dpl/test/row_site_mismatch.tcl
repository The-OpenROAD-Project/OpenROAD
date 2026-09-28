source "helpers.tcl"
# Regression test for a segfault in dpl::Grid::markHopeless().
#
# A row whose site spacing (STEP) is smaller than the site width declares
# overlapping sites.  The DEF reader now warns about such rows (ODB-0478), so to
# exercise the DPL guard here the malformed row is created programmatically
# after reading a valid floorplan.  DPL must report DPL-0052 instead of
# crashing with Signal 11.
read_lef Nangate45/Nangate45.lef
read_def row_site_mismatch.def

set db [ord::get_db]
set chip [$db getChip]
set block [$chip getBlock]
set lib [lindex [concat {*}[[$block getDataBase] getLibs]] 0]
set site [lindex [$lib getSites] 0]
odb::dbRow_create $block ROW_BAD $site 3800 2800 "R0" "HORIZONTAL" 32 140

catch { detailed_placement } msg
puts $msg
