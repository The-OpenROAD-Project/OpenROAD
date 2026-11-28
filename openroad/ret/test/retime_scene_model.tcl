# The search reads its delay model from the command scene alone.  Two scenes
# on one mode, a fast and a slow library: r1 stands in front of 16 inverters,
# which the fast scene fits in the period and the slow one does not.  With
# the fast scene as the command scene the model meets timing and nothing is
# proposed; with the slow one the model is the slow scene's, and its 14
# slides meet timing in both.
source "helpers.tcl"
set netlist [make_result_file retime_scene_model.v]
set stream [open $netlist w]
puts $stream "module top (a, b, clk, y);"
puts $stream "  input a;\n  input b;\n  input clk;\n  output y;"
puts $stream "  wire n0, q2;"
for { set i 0 } { $i <= 16 } { incr i } {
  puts $stream "  wire s$i;"
}
puts $stream "  AND2_X1 g0 (.A1(a), .A2(b), .ZN(n0));"
puts $stream "  DFF_X1 r1 (.D(n0), .CK(clk), .Q(s0));"
for { set i 1 } { $i <= 16 } { incr i } {
  puts $stream "  INV_X1 g$i (.A(s[expr { $i - 1 }]), .ZN(s$i));"
}
puts $stream "  DFF_X1 r2 (.D(s16), .CK(clk), .Q(q2));"
puts $stream "  BUF_X1 g17 (.A(q2), .Z(y));"
puts $stream "endmodule"
close $stream

read_liberty Nangate45/Nangate45_fast.lib
read_liberty Nangate45/Nangate45_slow.lib
read_lef Nangate45/Nangate45.lef
define_scene fast -liberty Nangate45/Nangate45_fast.lib
define_scene slow -liberty Nangate45/Nangate45_slow.lib

proc report_scenes { } {
  foreach name {fast slow} {
    set scene [sta::find_scene $name]
    puts "$name: worst slack [sta::format_time [sta::worst_slack_scene $scene max] 4]"
  }
}

foreach command_scene {fast slow} {
  read_verilog $netlist
  link_design top
  create_clock -period 0.46 clk
  set_false_path -from [get_ports {a b}]
  set_scene $command_scene
  puts "command scene $command_scene"
  report_scenes
  retime
  report_scenes
}
