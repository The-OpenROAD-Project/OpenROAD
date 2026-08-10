# Library-driven POCV (LVF-from-Liberty): set_pocv_sigma -from_liberty.
# OpenROAD-fork: LVF-lib
#
# Sources the per-stage delay sigma from the REAL Liberty LVF ocv_sigma_*
# tables of the loaded libraries (via OpenSTA's native PocvMode::normal delay
# calc) instead of one global hand-set -sigma. Sigma therefore varies per cell.
#
# Test data: a SYNTHETIC LVF library (lvf_lib.lib) -- Nangate45 ships no LVF
# tables -- with two buffer cells that share IDENTICAL mean delay tables but
# carry different ocv_sigma_cell_rise/fall magnitudes:
#   snl_bufhi : 0.0500 ns delay sigma (HIGH variation)
#   snl_buflo : 0.0050 ns delay sigma (LOW  variation, 10x smaller)
# lvf_sigma.v wires two independent 4-stage register-to-register chains, one all
# snl_bufhi, one all snl_buflo.
#
# Gates verified:
#  (1) FLAG OFF / -reset -> MEAN timing byte-identical to baseline (the feature
#      only adds variance to arrivals; it never shifts the mean).
#  (2) sta::pocv_liberty_has_lvf reports the library carries LVF tables.
#  (3) The HI and LO endpoints have the SAME mean slack (same mean delays).
#  (4) -from_liberty: the HI path accumulates a LARGER arrival sigma than the LO
#      path (sigma sourced per-cell from the library), so its worst-case
#      statistical slack (mean - n_sigma*sigma) is LESS optimistic (lower).
#  (5) Per-cell sourcing: 4 stages of 0.05 sigma accumulate in QUADRATURE to
#      ~0.10 (HI) and 4 stages of 0.005 to ~0.01 (LO), not a single global k.
source "helpers.tcl"
read_liberty lvf_lib.lib
read_lef lvf_lib.lef
read_verilog lvf_sigma.v
link_design top

create_clock -name clk -period 10 [get_ports clk]
set_input_delay 0 -clock clk [get_ports {in_hi in_lo}]
set_output_delay 0 -clock clk [get_ports {out_hi out_lo}]

# (2) library carries LVF tables
puts "has_lvf: [sta::pocv_liberty_has_lvf]"

# (1) baseline mean slack snapshot, then -from_liberty, then -reset: the MEAN
# slack of each endpoint must be unchanged (feature only adds variance).
set base_hi [get_property [get_pins capture_hi/D] slack_max]
set base_lo [get_property [get_pins capture_lo/D] slack_max]

# (3) HI and LO have identical mean slack (identical mean delay tables).
puts [format "means_equal: %d" [expr { abs($base_hi - $base_lo) < 1e-9 }]]

set_pocv_sigma -from_liberty -n_sigma 3

# (4)+(5) per-endpoint statistical slack: mean, accumulated sigma, stat slack.
set end_hi [lindex [find_timing_paths -path_delay max \
  -to [get_pins capture_hi/D] -group_path_count 1] 0]
lassign [sta::pocv_path_end_stat_slack $end_hi] hi_mean hi_sigma hi_stat hi_n

set end_lo [lindex [find_timing_paths -path_delay max \
  -to [get_pins capture_lo/D] -group_path_count 1] 0]
lassign [sta::pocv_path_end_stat_slack $end_lo] lo_mean lo_sigma lo_stat lo_n

# (1) MEAN slack unchanged by the feature. The library-driven mode runs under
# PocvMode::normal, whose mean delay calc differs from scalar mode only by
# floating-point rounding (~1e-9 rel), never a real timing shift. The
# BYTE-IDENTICAL flag-off gate is proven exactly by lvf_flagoff.tcl (report_checks
# string compare). Here we assert the statistical MEAN matches the scalar
# baseline to float precision.
puts [format "hi_mean_unchanged: %d" [expr { abs($hi_mean - $base_hi) < 1e-6 }]]
puts [format "lo_mean_unchanged: %d" [expr { abs($lo_mean - $base_lo) < 1e-6 }]]

# (4) HI sigma strictly larger than LO sigma (per-cell sourcing).
puts [format "hi_sigma_gt_lo_sigma: %d" [expr { $hi_sigma > $lo_sigma + 1e-9 }]]

# (4) HI worst-case statistical slack strictly LESS optimistic than LO.
puts [format "hi_stat_less_optimistic: %d" [expr { $hi_stat < $lo_stat - 1e-9 }]]

# (4) both stat slacks pulled below their (equal) mean by n_sigma*sigma.
puts [format "stat_below_mean: %d" \
  [expr { $hi_stat < $hi_mean - 1e-9 && $lo_stat < $lo_mean - 1e-9 }]]

# (5) QUADRATURE accumulation over the 4 buffer stages: per-stage sigma 0.05
# (HI) / 0.005 (LO) => sqrt(4)*sigma = 0.10 / 0.01. Allow tolerance for the
# flop CK->Q stage (no LVF sigma) contributing 0. Assert the ratio ~10x and the
# HI magnitude near 0.10.
puts [format "hi_sigma_quadrature: %d" [expr { abs($hi_sigma - 0.10) < 0.005 }]]
puts [format "lo_sigma_quadrature: %d" [expr { abs($lo_sigma - 0.01) < 0.005 }]]
puts [format "sigma_ratio_10x: %d" \
  [expr { abs($hi_sigma / $lo_sigma - 10.0) < 1.0 }]]

# (1) -reset returns to scalar delay-ops: mean slack identical to baseline.
set_pocv_sigma -reset
set rst_hi [get_property [get_pins capture_hi/D] slack_max]
set rst_lo [get_property [get_pins capture_lo/D] slack_max]
puts [format "reset_hi_baseline: %d" [expr { abs($rst_hi - $base_hi) < 1e-9 }]]
puts [format "reset_lo_baseline: %d" [expr { abs($rst_lo - $base_lo) < 1e-9 }]]
