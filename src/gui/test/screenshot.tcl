# test for creating a screenshot
source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def nangate_gcd/floorplan.def

set img [make_result_file screenshot.png]

save_image -resolution 0.100 -area [ord::get_die_area] $img
if { ![file exists $img] } {
  puts "Error: screenshot.png was not created"
} else {
  puts "screenshot.png created successfully"
}
