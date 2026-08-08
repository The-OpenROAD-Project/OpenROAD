# AOCV depth-based derate (dbSta first slice).
# Verifies:
#  (a) with no AOCV table loaded, AOCV-adjusted slack == flat slack (baseline),
#  (b) a depth-based late-derate table < 1.0 reduces late-path pessimism
#      (setup slack improves) in the expected direction.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def aocv_derate.def
read_sdc aocv_derate.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Apply a flat late derate so the baseline is pessimistic (like real OCV).
set_timing_derate -late 1.10

# --- (a) Feature inactive: AOCV slack must equal flat slack exactly ----------
set ends [find_timing_paths -path_delay max -group_path_count 1]
set end [lindex $ends 0]
set row [sta::aocv_adjust_path_end $end]
lassign $row endpoint depth flat aocv derate
puts [format "inactive: depth=%d flat=%.4f aocv=%.4f equal=%d" \
  $depth $flat $aocv [expr { abs($aocv - $flat) < 1e-9 }]]

# --- (b) Load a depth-based table: deeper paths get less pessimistic derate --
# Late derate decreases toward 1.0 as logic depth grows (variation averages
# out). For any depth >= 1 the derate (<1.10) is less than the flat 1.10, so
# the derated data delay shrinks and setup slack improves.
set_aocv_derate -depth 1 -late 1.08
set_aocv_derate -depth 3 -late 1.04
set_aocv_derate -depth 6 -late 1.01

set ends [find_timing_paths -path_delay max -group_path_count 1]
set end [lindex $ends 0]
set row [sta::aocv_adjust_path_end $end]
lassign $row endpoint depth flat aocv derate
set recovered [expr { $aocv - $flat }]
puts [format "aocv: depth=%d derate=%.2f flat=%.4f aocv=%.4f recovered=%.4f" \
  $depth $derate $flat $aocv $recovered]
puts [format "pessimism recovered (slack improved): %d" \
  [expr { $recovered > 0.0 }]]

# --- Reset returns to inactive (slack == flat again) -------------------------
set_aocv_derate -reset
set row [sta::aocv_adjust_path_end $end]
lassign $row endpoint depth flat aocv derate
puts [format "after reset: equal=%d" [expr { abs($aocv - $flat) < 1e-9 }]]
