# -max_moves caps the slides of the whole run, across rounds: the design of
# retime_dsec_deep makes 8 without a cap.  A negative cap and an argument are
# errors.  The result is proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_deep.v
link_design top
create_clock -period 0.6 clk

write_verilog_for_dsec retime_max_moves before
catch { retime -max_moves -1 } msg
puts $msg
catch { retime extra } msg
puts $msg

retime -max_moves 6
run_dsec_test retime_max_moves -liberty_files sky130hd/sky130hd_tt.lib   -clock clk -resetn rst_n
