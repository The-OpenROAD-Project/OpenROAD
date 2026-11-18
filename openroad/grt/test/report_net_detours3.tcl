# Test the detour metric on a design read from odb with global routing guides,
# before and after moving an instance of _309_ with incremental routing.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_db "sky130hs/gcd_grt.db"

report_net_detours -top_n 3

set block [ord::get_db_block]
set inst [[lindex [[$block findNet _309_] getITerms] 0] getInst]
set nets {}
foreach iterm [$inst getITerms] {
  set net [$iterm getNet]
  if { $net != "NULL" && ![$net isSpecial] } {
    lappend nets [$net getName]
  }
}
report_net_detours -net $nets

global_route -start_incremental
lassign [$inst getLocation] x y
$inst setLocation [expr { $x + 14400 }] $y
global_route -end_incremental

report_net_detours -net $nets
report_net_detours -top_n 3
