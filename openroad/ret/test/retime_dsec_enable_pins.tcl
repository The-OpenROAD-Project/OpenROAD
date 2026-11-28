# Enable pins, one cone each.  A gate whose output drives an enable pin is
# pinned, so no register crosses it: rE and rF stay.  A register whose
# output drives an enable pin fences its chain: rG (fenced) and rH stay.  A
# register that drives only the enable pin of another register keeps its
# depth, so the gate in front of it is never crossed: rc (fenced) stays.
# The path into it still fails, and the register behind that path slides up
# to that gate: i4 reads the new register.  The paths of these three cones
# still fail after retime.  A chain of a plain and an enable register that
# only loses its last register keeps the class of the register left behind:
# c2 reads a plain register, rebuilt from rA together with rD, which shares
# its data net, and the enable pair moves on as one enable register.  The
# result is proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_enable_pins.v
link_design top
create_clock -period 0.6 clk

write_verilog_for_dsec retime_dsec_enable_pins before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
report_kept {rE rF rG rH rc r1}
report_reads {i4/A c2/A c_t/A c5/A}
foreach pin {r3/D r13/D rc/D} {
  puts "$pin slack [format %.4f [get_property [get_pins $pin] slack_max]]"
}
run_dsec_test retime_dsec_enable_pins \
  -liberty_files sky130hd/sky130hd_tt.lib -clock clk -resetn rst_n
