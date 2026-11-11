# Negotiation phase 2 ends at its iteration limit and must hand the remaining
# illegal cells to diamond search.
# Each row has an overfilled pocket at the left die edge, walled off by fixed
# cells wider than the negotiation search window; free sites lie beyond the
# wall. The pockets keep the violation count changing, so neither stall
# detector fires.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def negotiation_iter_limit.def

detailed_placement
check "placement legal" {catch { check_placement }} 0

exit_summary
