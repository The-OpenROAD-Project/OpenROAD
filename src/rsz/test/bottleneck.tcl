source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog bottleneck.v
link_design top

#Create a virtual clock
create_clock -period 1.0 -name clk
set_input_delay -clock clk -max 0 [all_inputs]
set_output_delay -clock clk -max 1 [all_outputs]

report_checks

# Parameter alpha controls how individual paths are weighed.
# The following holds: Two paths going through a pin, each with slack S,
# contribute in total as much as a single path with slack S-alpha.
#
# Smaller alpha: critical paths dominate in determining bottleneck
# Higher alpha: the number of paths dominates in determining bottleneck
rsz::report_bottlenecks -npins 5 -alpha 0.1
