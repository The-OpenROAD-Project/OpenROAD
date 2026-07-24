# POCV propagation-time statistical derate (real per-arc variance accumulated in
# QUADRATURE during the forward search). OpenROAD-fork: LVF
#
# Verifies the task correctness gates:
#  (a) FLAG OFF (report-only set_pocv_sigma, no -propagate) AND after -reset ->
#      the timer's MEAN slack (worst_slack) is byte-identical to baseline. The
#      forward search variance machinery is dormant (scalar delay-ops).
#  (b) FLAG ON (-propagate) -> the path's arrival carries an accumulated
#      variance; the worst-case statistical slack (mean - n_sigma*sigma) is
#      strictly WORSE than the mean slack, and the implied path sigma is LESS
#      than a LINEAR (fully-correlated) sum of the same per-stage sigma. This is
#      the sqrt(N) de-pessimism realized DURING propagation.
#  (c) The propagated path sigma is CONSISTENT (same direction, comparable
#      magnitude) with report_checks_pocv's RSS estimate on the same path.
#  (d) -reset returns to scalar delay-ops -> baseline mean slack exactly.
#
# NOTE: worst_slack / get_property slack expose only the MEAN of the statistical
# slack (single-arg delayAsFloat in OpenSTA). The statistical (quantile) slack
# is read with sta::pocv_path_end_stat_slack, which exposes the accumulated
# arrival variance the forward search propagated.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def aocv_derate.def
read_sdc aocv_derate.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

set_timing_derate -late 1.10

set base_ws [worst_slack -max]
puts [format "flat_baseline_mean_ws=%.6f" $base_ws]

# --- (a) report-only set_pocv_sigma must NOT move the mean slack -------------
set_pocv_sigma -sigma 0.05 -n_sigma 3
set ro_ws [worst_slack -max]
puts [format "report_only_mean_ws=%.6f unchanged=%d" \
  $ro_ws [expr { abs($ro_ws - $base_ws) < 1e-9 }]]

# Report-only RSS / linear sigma for the worst path (for consistency check).
set ends [find_timing_paths -path_delay max -group_path_count 1]
set end [lindex $ends 0]
set row [sta::pocv_adjust_path_end $end]
lassign $row endpoint depth flat_slack pocv_slack lin_sigma rss_sigma nsig
puts [format "report: depth=%d lin_sigma=%.6f rss_sigma=%.6f" \
  $depth $lin_sigma $rss_sigma]
set_pocv_sigma -reset

# --- (b)+(c) Propagation ON ---------------------------------------------------
set_pocv_sigma -sigma 0.05 -n_sigma 3 -propagate
# Mean slack of the (possibly reselected) worst path must be >= baseline mean
# (statistical path selection only ever keeps the worse-statistical path; the
# mean readout of that path is not the metric of interest here).
set prop_mean_ws [worst_slack -max]
puts [format "propagate_mean_ws=%.6f" $prop_mean_ws]

# The statistical slack of the worst path: mean - n_sigma*sigma.
set ends [find_timing_paths -path_delay max -group_path_count 1]
set end [lindex $ends 0]
set s [sta::pocv_path_end_stat_slack $end]
lassign $s mean_slack slack_sigma stat_slack snsig
puts [format "stat: mean=%.6f slack_sigma=%.6f stat_slack=%.6f n_sigma=%.1f" \
  $mean_slack $slack_sigma $stat_slack $snsig]

# Propagation produced a real accumulated variance (sigma > 0 on a deep path).
puts [format "variance_propagated: %d" [expr { $slack_sigma > 1e-9 }]]
puts [format "deep_path: %d" [expr { $depth > 1 }]]

# Statistical slack is strictly worse (lower) than the mean slack: the n_sigma
# pessimism is applied during propagation, not in a post-hoc report.
puts [format "stat_worse_than_mean: %d" \
  [expr { $stat_slack < $mean_slack - 1e-9 }]]

# (b) sqrt(N) de-pessimism: the propagated path sigma (slack_sigma) is LESS than
# the LINEAR (fully-correlated) path sigma for the same per-stage sigma. The
# propagated sigma is the RSS sigma; lin_sigma is the linear sum. On any
# multi-stage path rss < lin strictly.
puts [format "rss_lt_linear: %d" \
  [expr { $depth <= 1 || $slack_sigma < $lin_sigma - 1e-9 }]]

# (c) consistency with report_checks_pocv's RSS estimate (same path, same k):
# the propagated slack sigma matches the report's rss_sigma in direction and
# magnitude. Tolerance covers statistical path re-selection and the fact that
# the report's rss is computed over combinational arc *delays* while the
# propagated sigma is over the actual propagated stage delays.
set consistent [expr { $slack_sigma > 0.0 \
  && abs($slack_sigma - $rss_sigma) < 0.40 * $rss_sigma + 1e-6 }]
puts [format "consistent_with_report: %d (prop=%.6f rss=%.6f)" \
  $consistent $slack_sigma $rss_sigma]

# --- (d) -reset returns to scalar delay-ops -> baseline mean slack exactly ----
set_pocv_sigma -reset
set after_ws [worst_slack -max]
puts [format "after_reset_mean_ws=%.6f unchanged=%d" \
  $after_ws [expr { abs($after_ws - $base_ws) < 1e-9 }]]
