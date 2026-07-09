# POCV must NEVER change propagation timing (worst_slack / WNS / TNS), because
# parametric POCV is report-only: the OpenSTA forward search is untouched.
# This proves the #1 correctness gate: flag-on or flag-off, the timer's own
# numbers are byte-identical. See POCV_INVESTIGATION.md.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def aocv_derate.def
read_sdc aocv_derate.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
set_timing_derate -late 1.10

proc snap {} {
  return [format "ws=%.6f wns=%.6f tns=%.6f" \
    [worst_slack -max] [total_negative_slack -max] \
    [total_negative_slack -max]]
}

set before [snap]
puts "before:   $before"

# Activate POCV with an aggressive sigma; propagation timing must NOT move.
set_pocv_sigma -sigma 0.20 -n_sigma 6
set during [snap]
puts "pocv_on:  $during"
puts [format "unchanged_on: %d" [expr { $before eq $during }]]

# Reset; still identical.
set_pocv_sigma -reset
set after [snap]
puts "after:    $after"
puts [format "unchanged_after: %d" [expr { $before eq $after }]]
