# A trial is kept only when no scene gets worse.  Two modes share the clock
# port: in A (period 0.13) the path from rb through g1 fails, in B (period
# 0.2, late input a) timing is met.  The slide across g1 would fix A and
# make the path from a fail in B, so no slide is kept.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_scenes.v
link_design top
set_mode A
create_clock -period 0.13 clk
set_input_delay 0.0 -clock clk [get_ports {a b rst_n}]
set_mode B
create_clock -period 0.20 clk
set_input_delay 0.0 -clock clk [get_ports {b rst_n}]
set_input_delay 0.115 -clock clk [get_ports a]
define_scene sA -mode A -liberty NangateOpenCellLibrary
define_scene sB -mode B -liberty NangateOpenCellLibrary
# The search reads the current scene: the failing one.
set_scene sA

proc report_scenes { } {
  foreach name {sA sB} {
    set scene [sta::find_scene $name]
    puts "$name: worst slack [sta::format_time [sta::worst_slack_scene $scene max] 4]"
  }
}

report_scenes
retime
report_scenes
