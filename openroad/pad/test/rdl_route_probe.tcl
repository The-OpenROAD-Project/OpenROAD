# Test for RDL router with a probe array
source "helpers.tcl"
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7_data/probe.lef

read_def asap7_data/rdl_route_probe.def

rdl_route -layer Pad -width 4 -spacing 4 "PROBE_*"

set def_file [make_result_file "rdl_route_probe.def"]
write_def $def_file
diff_files $def_file "rdl_route_probe.defok"
