# repair_timing -path_group restricts optimization to one path group
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def
create_clock -name core_clock -period 0.6 [get_ports clk]
set_input_delay -clock core_clock 0.05 [all_inputs -no_clocks]
set_output_delay -clock core_clock 0.3 [all_outputs]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

puts "path groups: [sta::path_group_names]"

# An unsupported name warns and leaves every path group eligible.
repair_timing -setup -path_group bogus -max_passes 1

# reg2out is not in the SDC, so repair_timing defines it.
repair_timing -setup -path_group reg2out -max_passes 1
puts "path groups: [sta::path_group_names]"
report_checks -path_group reg2out -digits 3 -path_delay max -fields {}

# Register endpoints only.
repair_timing -setup -path_group reg2reg -max_passes 1

# Hold repair honors the restriction too.
repair_timing -hold -hold_margin 0.4 -path_group reg2reg -max_passes 1

# Already defined, so no second group_path is made.
repair_timing -setup -path_group reg2reg -max_passes 1
puts "path groups: [sta::path_group_names]"
