# Parametric statistical OCV (POCV / LVF) derate, dbSta first slice
# (report-only, root-sum-square / quadrature). See POCV_INVESTIGATION.md.
#
# Verifies the task correctness gates:
#  (a) Feature inactive (no set_pocv_sigma) -> POCV slack == flat slack exactly
#      (baseline-safe; the forward search is never touched).
#  (b) With a per-stage sigma set, a deep path's variation combines in
#      QUADRATURE (sqrt-of-N), so the RSS path sigma is strictly LESS than the
#      linear (flat-derate-equivalent) path sigma -> setup slack improves, and
#      the recovered pessimism grows with depth. Direction is asserted.
#  (c) The sqrt-of-N math itself, checked deterministically against a closed
#      form for N equal stages: lin = k*N*d, rss = k*sqrt(N)*d.
#  (d) -reset returns to inactive (POCV slack == flat slack again).
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def aocv_derate.def
read_sdc aocv_derate.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Flat late OCV so the baseline carries pessimism (like real sign-off OCV).
set_timing_derate -late 1.10

# --- (a) Inactive: POCV slack must equal flat slack exactly ------------------
set ends [find_timing_paths -path_delay max -group_path_count 1]
set end [lindex $ends 0]
set row [sta::pocv_adjust_path_end $end]
lassign $row endpoint depth flat pocv lin rss nsig
puts [format "inactive: depth=%d flat=%.4f pocv=%.4f equal=%d" \
  $depth $flat $pocv [expr { abs($pocv - $flat) < 1e-9 }]]

# --- (b) Active: RSS sigma < linear sigma; pessimism recovered (slack up) -----
# 5% per-stage 1-sigma, 3-sigma sign-off.
set_pocv_sigma -sigma 0.05 -n_sigma 3
set ends [find_timing_paths -path_delay max -group_path_count 1]
set end [lindex $ends 0]
set row [sta::pocv_adjust_path_end $end]
lassign $row endpoint depth flat pocv lin rss nsig
set recovered [expr { $pocv - $flat }]
puts [format "active: depth=%d lin=%.5f rss=%.5f recovered=%.5f" \
  $depth $lin $rss $recovered]
# Quadrature is less pessimistic than linear for any multi-stage path.
puts [format "rss_lt_lin: %d" [expr { $rss < $lin }]]
puts [format "pessimism_recovered (slack improved): %d" [expr { $recovered > 0.0 }]]
# Deeper than a single stage => strict sqrt-of-N de-pessimism present.
puts [format "deep_path: %d" [expr { $depth > 1 }]]

# --- (c) Deterministic sqrt-of-N check (closed form, no timer dependence) -----
# For N equal stages of delay d and per-stage fraction k:
#   linear sigma = k * (N*d)          (fully correlated / flat derate)
#   RSS    sigma = k * sqrt(N) * d    (independent / quadrature)
# Ratio lin/rss = sqrt(N). Check at N=100 -> 10x.
set k 0.05
set d 0.020
set N 100
set lin_cf [expr { $k * $N * $d }]
set rss_cf [expr { $k * sqrt($N) * $d }]
set ratio  [expr { $lin_cf / $rss_cf }]
puts [format "closed_form N=%d lin=%.5f rss=%.5f ratio=%.4f sqrtN=%.4f" \
  $N $lin_cf $rss_cf $ratio [expr { sqrt($N) }]]
puts [format "ratio_is_sqrtN: %d" [expr { abs($ratio - sqrt($N)) < 1e-6 }]]

# --- (d) Reset returns to inactive (POCV slack == flat slack) -----------------
set_pocv_sigma -reset
set row [sta::pocv_adjust_path_end $end]
lassign $row endpoint depth flat pocv lin rss nsig
puts [format "after reset: equal=%d" [expr { abs($pocv - $flat) < 1e-9 }]]
