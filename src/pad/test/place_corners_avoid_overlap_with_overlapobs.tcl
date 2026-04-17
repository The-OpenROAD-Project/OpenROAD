# Test for assigning bumps
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

add_global_connection -pin_pattern "VDD" -net VDD -power
add_global_connection -pin_pattern "DVDD" -net DVDD -power
add_global_connection -pin_pattern "VSS" -net VSS -ground
add_global_connection -pin_pattern "DVSS" -net DVSS -ground

# Make IO Sites
make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 35

# Add inst in LL corner
place_inst -name "OVERLAP_PASS" -cell NOR3_X2 -origin "160 160" -status FIRM
place_inst -name "OVERLAP_REJECT" -cell NOR3_X2 -origin "2900 40" -status FIRM
place_inst -name "PASS_BUMP" -cell DUMMY_BUMP -origin "2900 2900" -status FIRM

# Place corners
place_corners PAD_CORNER_OBS

set def_file [make_result_file "place_corners_avoid_overlap_with_overlapobs.def"]
write_def $def_file
diff_files $def_file "place_corners_avoid_overlap_with_overlapobs.defok"
