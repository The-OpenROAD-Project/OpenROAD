# Tests a hierarchical name in def "x0/d0" against a hierarchical name in Verilog
# \x0/d0.  We allow such matches as a fallback if the hierarchical name fails to
# match.

source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_verilog data/floorplan_initialize2.v
link_design counter
read_def -floorplan_initialize "data/floorplan_initialize2.def"
set chip [$db getChip]
if {$chip == "NULL"} {
    puts "FAIL: Read DEF Failed"
    exit 1
}

set block [$chip getBlock]
set out_def [make_result_file "floorplan_initialize2.def"]
write_def $out_def

diff_files $out_def "floorplan_initialize2.defok"

puts "pass"
#exit 0
