# buffer output with assign between output ports
source "helpers.tcl"
source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog buffer_ports8.v
link_design top

# Load the placed fixture to keep this test independent of floorplan and placement.
read_def -floorplan_initialize buffer_ports8.def

buffer_ports -inputs -outputs

set def_file [make_result_file buffer_ports8.def]
write_def $def_file
diff_files buffer_ports8.defok $def_file
