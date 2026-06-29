# Test the read-only routing detour metric: ratio of final routed wirelength
# to the initial Steiner-tree wirelength captured before overflow removal.
# Uses gcd_nangate45 (same design as gcd.tcl) so routing is unchanged.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

global_route

report_net_detours -top_n 5
