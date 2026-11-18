source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog repair_clk_inverters2_hier.v
link_design -hier top
create_clock -period 1 clk

set constrained_pin [get_pins m1/u_regs/clk_n]
puts "Constrained hierarchical pins: [llength $constrained_pin]"
set_false_path -through $constrained_pin

repair_clock_inverters
report_net clk
puts "Original inverter count: [llength [get_cells -quiet m1/c1]]"
