# One register crosses 1999 gates in one round: r1 stands in front of 2000
# inverters in series and the period fits one of them.  The register each
# crossing leaves is traced back to r1 through every gate crossed before it.
source "helpers.tcl"
set netlist [make_result_file retime_gate_chain.v]
set stream [open $netlist w]
puts $stream "module top (a, b, clk, y);"
puts $stream "  input a;\n  input b;\n  input clk;\n  output y;"
puts $stream "  wire n0, q2;"
for { set i 0 } { $i <= 2000 } { incr i } {
  puts $stream "  wire s$i;"
}
puts $stream "  AND2_X1 g0 (.A1(a), .A2(b), .ZN(n0));"
puts $stream "  DFF_X1 r1 (.D(n0), .CK(clk), .Q(s0));"
# Listed from the last gate to the first: the gates are not in path order.
for { set i 2000 } { $i >= 1 } { incr i -1 } {
  puts $stream "  INV_X1 g$i (.A(s[expr { $i - 1 }]), .ZN(s$i));"
}
puts $stream "  DFF_X1 r2 (.D(s2000), .CK(clk), .Q(q2));"
puts $stream "  BUF_X1 g2001 (.A(q2), .Z(y));"
puts $stream "endmodule"
close $stream
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog $netlist
link_design top
create_clock -period 0.13 clk
set_false_path -from [get_ports {a b}]

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
