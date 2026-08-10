# AOCV propagation-time depth derate (real per-arc derate during search).
# OpenROAD-fork: AOCV
#
# Verifies the four correctness gates from the task:
#  (a) FLAG OFF / no table -> timing byte-identical to flat baseline.
#  (b) FLAG ON with a FLAT table (all depths -> same factor) reproduces the
#      flat set_timing_derate worst slack for that factor.
#  (c) FLAG ON with a depth-DECREASING table reduces late-path pessimism
#      (worst setup slack improves vs the flat OCV result).
#  (d) -no_propagate / -reset returns to the flat baseline exactly.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def aocv_derate.def
read_sdc aocv_derate.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

proc worst_slack_max {} {
  # Worst (min) setup slack across max-delay paths, in user time units.
  return [worst_slack -max]
}

# Flat late OCV baseline (pessimistic, like real OCV).
set_timing_derate -late 1.10
set flat_baseline [worst_slack_max]
puts [format "flat_baseline_max_slack=%.5f" $flat_baseline]

# --- (a) Propagation OFF: a loaded table must NOT change timing -------------
# Load a depth table but do NOT enable propagation; worst slack must be the
# flat baseline exactly (report-only path is unaffected by propagation flag).
set_aocv_derate -depth 1 -late 1.08
set_aocv_derate -depth 3 -late 1.04
set_aocv_derate -depth 6 -late 1.01
set off_slack [worst_slack_max]
puts [format "prop_off_max_slack=%.5f equal_baseline=%d" \
  $off_slack [expr { abs($off_slack - $flat_baseline) < 1e-9 }]]

# --- (b) Propagation ON with a FLAT table == flat set_timing_derate ----------
# Clear, install a single-depth (depth 1) table at 1.10 covering all depths,
# enable propagation. Result must match the flat 1.10 baseline.
set_aocv_derate -reset
set_aocv_derate -depth 1 -late 1.10 -propagate
set flat_table_slack [worst_slack_max]
puts [format "prop_on_flat_table_max_slack=%.5f equal_baseline=%d" \
  $flat_table_slack [expr { abs($flat_table_slack - $flat_baseline) < 1e-9 }]]

# --- (c) Propagation ON with depth-DECREASING table reduces pessimism --------
set_aocv_derate -reset
set_aocv_derate -depth 1 -late 1.10 -propagate
set_aocv_derate -depth 2 -late 1.06
set_aocv_derate -depth 4 -late 1.03
set_aocv_derate -depth 8 -late 1.00
set aocv_slack [worst_slack_max]
set recovered [expr { $aocv_slack - $flat_baseline }]
puts [format "prop_on_aocv_max_slack=%.5f recovered=%.5f improved=%d" \
  $aocv_slack $recovered [expr { $recovered > 0.0 }]]

# --- (d) -no_propagate returns to flat baseline exactly ----------------------
set_aocv_derate -no_propagate
set restored [worst_slack_max]
puts [format "prop_disabled_max_slack=%.5f equal_baseline=%d" \
  $restored [expr { abs($restored - $flat_baseline) < 1e-9 }]]

# -reset clears everything; still baseline.
set_aocv_derate -reset
set reset_slack [worst_slack_max]
puts [format "after_reset_max_slack=%.5f equal_baseline=%d" \
  $reset_slack [expr { abs($reset_slack - $flat_baseline) < 1e-9 }]]
