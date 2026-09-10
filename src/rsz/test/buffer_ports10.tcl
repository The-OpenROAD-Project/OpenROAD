# buffer input/output ports with detailed messages
source "helpers.tcl"
source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog buffer_ports10.v
link_design top

# Load the placed fixture to keep this test independent of floorplan and placement.
read_def -floorplan_initialize buffer_ports10.def

buffer_ports -inputs -outputs -verbose

set def_file [make_result_file buffer_ports10.def]
write_def $def_file
diff_files buffer_ports10.defok $def_file
