# A transparent latch in the design is don't-touch, not an error; the register
# chain next to it is still retimed.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_latch.v
link_design top
create_clock -period 1.0 clk

retime

set v_file [make_result_file retime_latch.v]
write_verilog $v_file
diff_files retime_latch.vok $v_file
