# power for reg with sequential internal pins
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog power1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}

set_power_activity -input -activity .1
report_power -instance r1
puts [get_property [get_pins r1/Q] activity]
