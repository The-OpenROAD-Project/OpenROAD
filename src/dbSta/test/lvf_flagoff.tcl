# Library-driven POCV (-from_liberty) FLAG-OFF correctness gate. OpenROAD-fork: LVF-lib
#
# THE #1 GATE: when the feature is inactive the timer's numbers must be
# byte-identical to baseline. Nangate45 ships no LVF (ocv_sigma_*) tables, so
# `set_pocv_sigma -from_liberty` is inert here (a warning is issued and the
# statistical variance is zero). This test captures the FULL report_checks text
# before activating -from_liberty, after activating it (no LVF => baseline), and
# after -reset, and asserts all three are byte-for-byte identical. Uses the gcd
# (aocv_derate.def) setup.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def aocv_derate.def
read_sdc aocv_derate.sdc
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Capture full report_checks BEFORE touching the feature.
set before [report_checks -path_delay max -format full_clock_expanded -fields {slew cap input_pins} -digits 6]

# Activate -from_liberty. Nangate45 has NO LVF tables, so the feature is inert
# (warning emitted, zero variance) -- timing MUST be byte-identical to baseline.
set_pocv_sigma -from_liberty -n_sigma 3
set during [report_checks -path_delay max -format full_clock_expanded -fields {slew cap input_pins} -digits 6]

# Reset back to scalar delay-ops.
set_pocv_sigma -reset
set after [report_checks -path_delay max -format full_clock_expanded -fields {slew cap input_pins} -digits 6]

puts "from_liberty_no_lvf_identical: [expr { $before eq $during }]"
puts "after_reset_identical: [expr { $before eq $after }]"
