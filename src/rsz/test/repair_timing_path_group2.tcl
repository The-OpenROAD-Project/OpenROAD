# repair_timing -path_group separates clock gating checks from reg2reg
source "helpers.tcl"
source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def inferred_clock_gator_time_borrow.def

create_clock -name clk -period 1.0 clk
create_clock -name vclk -period 1.0
set_input_delay -clock vclk 0.98 [get_ports en_in]
set_output_delay -clock clk 0.1 [all_outputs]

source Nangate45/Nangate45.rc
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
set_propagated_clock [all_clocks]
estimate_parasitics -placement

# The clock gating check OpenSTA reports separately from the register groups.
# -path_group takes a list, and the built-in name has a space in it.
report_checks -path_group [list "gated clock"] -digits 4 -fields {} \
  -path_delay max

# Every violating endpoint must land in some group, with the gated clock enable
# landing in gated_clock and not in reg2reg. The groups may overlap, since an
# endpoint fed by both a primary input and a register violates in in2reg and
# reg2reg at once, so the per group counts can add up to more than the
# unrestricted count.
#
# -max_passes 0 plus the skip flags collects the violating endpoints without
# committing any move, so every count below is measured against the same
# untouched netlist.
set collect_only {-max_passes 0 -skip_last_gasp -skip_crit_vt_swap -skip_vt_swap}
foreach path_group {reg2reg in2reg reg2out in2out gated_clock} {
  puts "-- $path_group"
  repair_timing -setup -path_group $path_group {*}$collect_only
}
puts "-- unrestricted"
repair_timing -setup {*}$collect_only

puts "path groups: [sta::path_group_names]"
