# Check if hierarchical slash escaped names are correctly written in Verilog and SPEF.
source "helpers.tcl"

set test_name escape_slash_hier

source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog $test_name.v
link_design -hier multi_sink
read_def -floorplan_initialize $test_name.def

source Nangate45/Nangate45.rc
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk

set spef_file [make_result_file $test_name.spef]
estimate_parasitics -placement -spef_file $spef_file
diff_files $test_name.spefok $spef_file

set vout_file [make_result_file $test_name.v]
write_verilog $vout_file
diff_files $test_name.vok $vout_file
