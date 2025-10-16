# Test for RDL router with bump only routing
source "helpers.tcl"
read_lef passive_tech/tech.lef
read_lef passive_tech/bumps.lef

read_def passive_tech/floorplan.def

rdl_route -layer topmetal -width 4 -spacing 4 ios0*

set def_file [make_result_file "rdl_route_bump_to_bump_only.def"]
write_def $def_file
diff_files $def_file "rdl_route_bump_to_bump_only.defok"
