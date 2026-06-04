# Verify analyze_power_grid handles a backside-power-delivery design.
#
# Regression test for:
#   * isBackside()-aware ITerm base-layer selection in
#     src/psm/src/ir_network.cpp (without this fix every ITerm reports
#     PSM-0039 "Unconnected instance" on a BSPDN PDK because the
#     virtual base node lands on a backside layer that has no shapes).
#   * Multi-seed connectivity BFS in src/psm/src/ir_solver.cpp (without
#     this fix the BFS misses subgraphs that only touch the top layer
#     through bridge cells, flagging thousands of spurious PSM-0038
#     "Unconnected shape" warnings).
#
# Design (in backside_data/):
#   * backside.lef declares one routing layer B1 marked LEF58_BACKSIDE
#     plus a normal M1, two macros: bridge_tap (vdd / vss on both M1
#     and B1, modeling a TSV-like front<->back bridge cell) and
#     backside_filler (vdd / vss only on B1, modeling a normal cell on
#     a BSPDN PDK).
#   * backside.def places two bridge taps and three fillers along one
#     row, with B1 followpins running across the row and a pair of
#     M1 stripes through each tap.
#
# Without the two fixes above check_power_grid floods the log with
# PSM-0038 and PSM-0039; with them it reports a single PSM-0040
# "All shapes ... are connected" for each net.
source helpers.tcl

read_lef backside_data/backside.lef
read_def backside_data/backside.def

check_power_grid -net vdd -dont_require_terminals
check_power_grid -net vss -dont_require_terminals
