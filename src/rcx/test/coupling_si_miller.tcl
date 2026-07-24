# Crosstalk / SI first slice: coupling Miller-factor derate.
#
# Verifies on gcd (sky130hs) with real extracted coupling caps that:
#   (a) Miller factor 1.0 reproduces the baseline timing exactly,
#   (b) Miller factor > 1.0 (worst-case opposite-switching aggressor)
#       increases the worst-case setup path delay (TNS more negative),
#   (c) restoring factor 1.0 returns to the exact baseline, and
#   (d) report_coupling_si ranks coupled nets.
#
# The coupling caps must be KEPT in the parasitic network for the factor to
# have any effect (read_spef -keep_capacitive_coupling); this is the path the
# Miller factor scales in OpenSTA's pi/Arnoldi reduction.
source helpers.tcl

read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_liberty sky130hs/sky130hs_tt.lib

read_def gcd.def

# Load via resistance info
source sky130hs/sky130hs.rc

define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file ext_pattern.rules -max_res 0 \
  -coupling_threshold 0.1

set spef_file [make_result_file coupling_si_miller.spef]
write_spef $spef_file

# Keep coupling caps so the Miller factor has something to scale.
read_spef -keep_capacitive_coupling $spef_file

# Tight clock so the setup paths are constrained and coupling-sensitive.
create_clock -name clk -period 1.0 [get_ports clk]

proc setup_path_metric {} {
  # Total negative setup slack aggregates every endpoint, so any increase in
  # data-path delay from the coupling derate is reflected here.
  return [sta::total_negative_slack_cmd "max"]
}

# ---- (a) baseline: factor 1.0 == no change ----
set_coupling_miller_factor -setup 1.0 -hold 1.0
set base [setup_path_metric]
puts "baseline TNS (mf=1.0): [format %.6e $base]"

# ---- (b) worst-case Miller derate: factor 2.0 should slow setup ----
set_coupling_miller_factor -setup 2.0 -hold 1.0
set derate [setup_path_metric]
puts "derated TNS (mf=2.0): [format %.6e $derate]"

if { $derate < $base } {
  puts "PASS: mf=2.0 increases worst-case setup path delay (TNS decreased)"
} else {
  puts "FAIL: mf=2.0 did not increase setup path delay\
        (base=$base derate=$derate)"
}

# ---- (c) restore factor 1.0 -> back to baseline exactly ----
set_coupling_miller_factor -setup 1.0 -hold 1.0
set restored [setup_path_metric]
if { $restored == $base } {
  puts "PASS: mf=1.0 reproduces baseline exactly (restored == baseline)"
} else {
  puts "FAIL: restoring mf=1.0 did not return to baseline\
        (base=$base restored=$restored)"
}

# ---- (d) SI hotspot report ----
report_coupling_si -max_nets 5
