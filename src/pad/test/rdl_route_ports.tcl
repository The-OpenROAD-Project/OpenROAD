# Test for RDL router without 45* using ports instead of nets
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan_flipchip.def

rdl_route -layer metal10 -width 4 -spacing 4 [get_ports "VDD DVDD VSS DVSS p_*"]

set def_file [make_result_file "rdl_route_ports.def"]
write_def $def_file
diff_files $def_file "rdl_route_ports.defok"
