# Test for RDL router skipping with "RDL_ROUTE"
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan_flipchip.def

odb::dbBoolProperty_create [[ord::get_db_block] findITerm BUMP_2_4/PAD] RDL_ROUTE 1

rdl_route -layer metal10 -width 4 -spacing 4 "VDD"

set def_file [make_result_file "rdl_route_skip_iterm.def"]
write_def $def_file
diff_files $def_file "rdl_route_skip_iterm.defok"
