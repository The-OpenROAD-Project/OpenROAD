# No legal forward move: retime reports that it found no slides and changes nothing.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_no_moves.v
link_design top
create_clock -period 1.0 clk

retime

set v_file [make_result_file retime_no_moves.v]
write_verilog $v_file
diff_files retime_no_moves.vok $v_file
