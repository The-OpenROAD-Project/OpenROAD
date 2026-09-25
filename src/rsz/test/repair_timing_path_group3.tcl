# repair_timing -path_group sees a group's paths even when the endpoint's
# overall worst path belongs to a different group.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def

# The large input delay makes the in2reg path the worst one at every register
# endpoint, while violating reg2reg paths remain underneath it. Classifying an
# endpoint by its single worst path would drop all of reg2reg here.
create_clock -name core_clock -period 0.65 [get_ports clk]
set_input_delay -clock core_clock 0.50 [all_inputs -no_clocks]
set_output_delay -clock core_clock 0.1 [all_outputs]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

rsz::resolve_path_group reg2reg

# The endpoints overlap: in2reg holds every register endpoint, reg2reg holds
# the subset that also violates from a register, so the counts add up to more
# than the unrestricted run sees.
set collect_only {-max_passes 0 -skip_last_gasp -skip_crit_vt_swap -skip_vt_swap}
foreach path_group {reg2reg in2reg} {
  puts "-- $path_group"
  repair_timing -setup -path_group $path_group {*}$collect_only
}
puts "-- unrestricted"
repair_timing -setup {*}$collect_only

# repair_timing's reg2reg WNS must agree with what report_checks reports for
# the same group, both before and after the repair.
puts "-- reg2reg before"
report_checks -path_group reg2reg -path_delay max -digits 4 -fields {} \
  -group_path_count 1 -format summary
repair_timing -setup -path_group reg2reg
puts "-- reg2reg after"
report_checks -path_group reg2reg -path_delay max -digits 4 -fields {} \
  -group_path_count 1 -format summary
