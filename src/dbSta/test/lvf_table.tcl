# Table-indexed (load-dependent) library-driven POCV. OpenROAD-fork: LVF-full
#
# Proves set_pocv_sigma -from_liberty reads the per-arc delay sigma as a TABLE
# LOOKUP over the SAME slew x load axes as the nominal delay table -- i.e. real
# table-indexed LVF -- not a single per-arc constant. The prior lvf_sigma.tcl
# used scalar (axis-less) ocv_sigma tables; this test uses the 2-D
# ocv_sigma_cell_rise/fall tables of snl_bufvar, whose sigma grows with output
# load.
#
# Setup: two identical single-stage snl_bufvar paths. The bufvar output net of
# the LARGE path is given a much bigger capacitive load than the SMALL path via
# set_load, so the two arcs land at different load points of the sigma table.
#
# Gates:
#  (1) library carries LVF tables (pocv_liberty_has_lvf).
#  (2) -from_liberty: the LARGE-load arc accumulates a STRICTLY LARGER arrival
#      sigma than the SMALL-load arc, sourced from the 2-D sigma table. A scalar
#      sigma would give equal sigma -- so this asserts the table-indexed lookup.
#  (3) the larger-sigma (large-load) path is LESS optimistic (lower stat slack
#      relative to its own mean by more sigma margin).
#  (4) -reset returns to scalar delay-ops with mean slack unchanged (baseline).
source "helpers.tcl"
read_liberty lvf_lib.lib
read_lef lvf_lib.lef
read_verilog lvf_table.v
link_design top

create_clock -name clk -period 10 [get_ports clk]
set_input_delay 0 -clock clk [get_ports {in_sm in_lg}]
set_output_delay 0 -clock clk [get_ports {out_sm out_lg}]

# Force the two bufvar arcs onto different points of the load axis. The SMALL
# path keeps a tiny load (near index_2[0]); the LARGE path gets a big load
# (near the top of the table, index_2[5]=0.500 pf).
set_load 0.005 [get_nets nsm]
set_load 0.450 [get_nets nlg]

# (1) library carries LVF (ocv_sigma_*) tables.
puts "has_lvf: [sta::pocv_liberty_has_lvf]"

# Baseline (scalar) mean slack snapshot BEFORE enabling the feature.
set base_sm [get_property [get_pins capture_sm/D] slack_max]

set_pocv_sigma -from_liberty -n_sigma 3

# Per-endpoint statistical readout: mean, accumulated arrival sigma, stat slack.
set end_sm [lindex [find_timing_paths -path_delay max \
  -to [get_pins capture_sm/D] -group_path_count 1] 0]
lassign [sta::pocv_path_end_stat_slack $end_sm] sm_mean sm_sigma sm_stat sm_n

set end_lg [lindex [find_timing_paths -path_delay max \
  -to [get_pins capture_lg/D] -group_path_count 1] 0]
lassign [sta::pocv_path_end_stat_slack $end_lg] lg_mean lg_sigma lg_stat lg_n

# (2) THE table-indexed gate: same cell, larger output load => larger sigma
# (read from the 2-D ocv_sigma_cell_* table). A scalar sigma => equality.
puts [format "lg_sigma_gt_sm_sigma: %d" [expr { $lg_sigma > $sm_sigma + 1e-6 }]]

# (2b) both arcs carry NON-zero library sigma (sanity: the table was read).
puts [format "both_sigma_nonzero: %d" \
  [expr { $sm_sigma > 1e-9 && $lg_sigma > 1e-9 }]]

# (3) the large-load arc is pulled further below its mean (more sigma margin).
puts [format "lg_margin_gt_sm_margin: %d" \
  [expr { ($lg_mean - $lg_stat) > ($sm_mean - $sm_stat) + 1e-6 }]]

# (4) -reset: scalar delay-ops, mean slack identical to the pre-feature baseline.
set_pocv_sigma -reset
set rst_sm [get_property [get_pins capture_sm/D] slack_max]
puts [format "reset_mean_unchanged: %d" [expr { abs($rst_sm - $base_sm) < 1e-9 }]]
