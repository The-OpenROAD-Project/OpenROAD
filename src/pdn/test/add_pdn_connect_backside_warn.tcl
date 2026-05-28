# Verify add_pdn_connect warns when its two layers span the front-side
# / backside boundary. A standard cut-layer via cannot bridge a normal
# routing metal to a LEF58_BACKSIDE layer; the connection has to come
# from a tap cell, so we warn the user.
#
# Regression test for the PDN warning added on the bspdn branch
# (commit b55936a1ca).
source "helpers.tcl"

read_lef ../../psm/test/backside_data/backside.lef
read_def ../../psm/test/backside_data/backside.def

add_global_connection -net vdd -pin_pattern "^vdd$" -power
add_global_connection -net vss -pin_pattern "^vss$" -ground

define_pdn_grid -name {grid} -voltage_domains {CORE} -pins {B1}
add_pdn_stripe -grid {grid} -layer {B1} -width {0.07} -pitch {1.4} -followpins
# This add_pdn_connect crosses the front-side / backside boundary
# (B1 has LEF58_BACKSIDE, M1 doesn't). Should trigger PDN-1200.
add_pdn_connect -grid {grid} -layers {M1 B1}
