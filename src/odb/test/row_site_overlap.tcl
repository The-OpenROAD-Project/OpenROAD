# Regression test: rows whose site spacing is smaller than the site extent
# along the row declare overlapping sites.  The DEF reader ignores them and
# dbRow::create rejects them.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
catch { read_def row_site_overlap.def } msg
puts $msg

set block [ord::get_db_block]
puts "rows: [llength [$block getRows]]"

set lib [lindex [[ord::get_db] getLibs] 0]
set site [$lib findSite FreePDK45_38x28_10R_NP_162NW_34O]
puts "site: [$site getWidth] x [$site getHeight]"

# Spacing equal to the oriented site extent is accepted.
odb::dbRow_create $block ROW_H $site 0 0 "R0" "HORIZONTAL" 4 380
odb::dbRow_create $block ROW_V $site 0 0 "R90" "VERTICAL" 4 380
odb::dbRow_create $block ROW_HR90 $site 0 0 "R90" "HORIZONTAL" 4 2800
puts "rows: [llength [$block getRows]]"

# Spacing smaller than the oriented site extent is rejected.
catch { odb::dbRow_create $block ROW_BAD_H $site 0 0 "R0" "HORIZONTAL" 4 140 } msg
puts $msg
catch { odb::dbRow_create $block ROW_BAD_V $site 0 0 "R0" "VERTICAL" 4 380 } msg
puts $msg
puts "rows: [llength [$block getRows]]"
