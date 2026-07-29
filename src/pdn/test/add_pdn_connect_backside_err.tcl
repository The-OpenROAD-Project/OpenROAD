# Verify add_pdn_connect errors out when its two layers span the
# front-side / backside boundary. A standard cut-layer via cannot
# bridge a normal routing metal to a LEF58_BACKSIDE layer; PDN cannot
# create TSVs, so the connection has to come from a tap/bridge cell.
#
# Regression test for the PDN-1200 error added on the bspdn branch.
source "helpers.tcl"

read_lef backside_data/backside.lef
read_def backside_data/backside.def

add_global_connection -net vdd -pin_pattern "^vdd$" -power
add_global_connection -net vss -pin_pattern "^vss$" -ground

define_pdn_grid -name {grid} -voltage_domains {CORE} -pins {B1}
add_pdn_stripe -grid {grid} -layer {B1} -width {0.07} -pitch {1.4} -followpins
# This add_pdn_connect crosses the front-side / backside boundary
# (B1 has LEF58_BACKSIDE, M1 doesn't). Should trigger PDN-1200.
catch { add_pdn_connect -grid {grid} -layers {M1 B1} } err
puts $err
