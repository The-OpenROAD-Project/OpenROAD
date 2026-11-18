# One clock with a gated copy: retime runs, the gated chain is retimed with its
# new register on the gated net, and the gate fed from both clock nets is pinned.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_gated_clock.v
link_design top
create_clock -period 1.0 clk

retime

set v_file [make_result_file retime_gated_clock.v]
write_verilog $v_file
diff_files retime_gated_clock.vok $v_file
