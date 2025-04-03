# after reading a polygonal floorplan ensure that virtual blockages are
# not in the written def
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def init_floorplan_polygon.def

set def_file [make_result_file init_floorplan_polygon.def]
write_def $def_file
diff_files init_floorplan_polygon.defok $def_file

