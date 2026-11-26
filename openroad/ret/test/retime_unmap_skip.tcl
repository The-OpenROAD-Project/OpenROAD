# A register the unmapper skips (sky130hd).  r3 is an enable register with
# its DE pin unconnected; the slide across g20 merges it with the plain r4
# and would unmap it, but its next state cannot be rebuilt (RET-0009).  Each
# trial with that slide is not applied, and the shorter prefix that only
# slides the plain cone is kept.  Round 2's ranked list starts with the
# path through r3, so none of its prefixes is applied.  r3 stays an edfxtp
# and nothing is unmapped.  The floating DE makes sequential equivalence
# meaningless, so the netlist is compared with a golden one.
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_unmap_skip.v
link_design top
create_clock -period 0.5 clk

report_worst_slack -max -digits 4
report_tns -digits 4
retime
report_worst_slack -max -digits 4
report_tns -digits 4

set v_file [make_result_file retime_unmap_skip.v]
write_verilog $v_file
diff_files retime_unmap_skip.vok $v_file
