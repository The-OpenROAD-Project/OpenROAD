# A register that moves more than one gate deep keeps the clock net of the
# register it replaces, here the gated one, and gates fed from a net whose
# registers are on different clock nets are pinned.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_gated_clock_deep.v
link_design top
create_clock -period 0.1 clk

retime

set v_file [make_result_file retime_gated_clock_deep.v]
write_verilog $v_file
diff_files retime_gated_clock_deep.vok $v_file
