# Registers at ports, one cone each.  A register fed straight from an input
# port has no gate in front of it, so the gate that reads it is never
# crossed: rp and r1 stay.  A primary output tapped from the middle of a
# register chain fences that chain: rA (fenced), rB, rC and rD stay.  A
# register that drives only an output port keeps its depth, so the gate in
# front of it is never crossed: rz (fenced) stays.  The path into it still
# fails, and the register behind that path slides up to that gate: i4 reads
# the new register.  The paths of these three cones still fail after
# retime.  The fourth cone has none of these and retimes: r4 and r5 are
# removed.  The result is proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_port.v
link_design top
create_clock -period 0.6 clk

write_verilog_for_dsec retime_dsec_port before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
report_kept {rp r1 rA rB rC rD rz r8 r4 r5}
report_reads i4/A
foreach pin {r3/D r7/D rz/D} {
  puts "$pin slack [format %.4f [get_property [get_pins $pin] slack_max]]"
}
run_dsec_test retime_dsec_port -liberty_files sky130hd/sky130hd_tt.lib \
  -clock clk -resetn rst_n
