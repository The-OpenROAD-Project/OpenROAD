# Regression test for #12401: group_path -through a net in a hierarchical
# design must keep matching paths when a module pin is reconnected flat-net
# first or swapped by repair_timing.
source "helpers.tcl"

proc group_endpoints { group } {
  set endpoints {}
  foreach pe [find_timing_paths -path_group $group -path_delay max \
    -group_path_count 100 -endpoint_path_count 1 \
    -unique_paths_to_endpoint] {
    lappend endpoints [get_full_name [get_property $pe endpoint]]
  }
  return [lsort -unique $endpoints]
}

proc iterm_mod_name { iterm } {
  return [[$iterm getModNet] getHierarchicalName]
}

# Case 1: direct ODB flat-net-first reconnect ($iterm connect $flat_net;
# $iterm connect $mod_net) on a module net and a top net crossing the module.
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog hier_thru_pin_swap.v
link_design -hier top
create_clock -name clk -period 0.1 [get_ports clk]

group_path -name g -through [get_nets u/b]
group_path -name g_top -through [get_nets n_thru]

check "before direct swap: g endpoints" {group_endpoints g} {u/r2/D}
check "before direct swap: g_top endpoints" {group_endpoints g_top} {r4/D u/r2/D}

set block [ord::get_db_block]
set a [$block findITerm u/g/A]
set b [$block findITerm u/g/B]
set a_net [$a getNet]
set a_mod [$a getModNet]
set b_net [$b getNet]
set b_mod [$b getModNet]

$a disconnect
$a connect $b_net
$a connect $b_mod
$b disconnect
$b connect $a_net
$b connect $a_mod

group_path -name g_fresh -through [get_nets u/b]
group_path -name g_top_fresh -through [get_nets n_thru]

check "after direct swap: g matches fresh" \
  {group_endpoints g} [group_endpoints g_fresh]
check "after direct swap: g endpoints" {group_endpoints g} {u/r2/D}
check "after direct swap: g_top matches fresh" \
  {group_endpoints g_top} [group_endpoints g_top_fresh]
check "after direct swap: g_top endpoints" \
  {group_endpoints g_top} {r4/D u/r2/D}

# Swap back so repair_timing sees the late n_thru cone on the slower B pin.
$a disconnect
$a connect $a_net
$a connect $a_mod
$b disconnect
$b connect $b_net
$b connect $b_mod

check "after swap-back: u/g/B restored to u/b" \
  {iterm_mod_name $b} "u/b"
check "after swap-back: g endpoints" {group_endpoints g} {u/r2/D}

# Case 2: repair_timing pin swap (Resizer::swapPins -> dbNetwork::connectPin).
source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
estimate_parasitics -placement

repair_timing -setup -sequence swap

check "repair_timing swapped u/g/A onto u/b" \
  {iterm_mod_name $a} "u/b"
check "repair_timing swapped u/g/B onto u/a" \
  {iterm_mod_name $b} "u/a"

group_path -name g_after_rsz -through [get_nets u/b]
group_path -name g_top_after_rsz -through [get_nets n_thru]

check "after repair_timing swap: g matches fresh" \
  {group_endpoints g} [group_endpoints g_after_rsz]
check "after repair_timing swap: g endpoints" {group_endpoints g} {u/r2/D}
check "after repair_timing swap: g_top matches fresh" \
  {group_endpoints g_top} [group_endpoints g_top_after_rsz]
check "after repair_timing swap: g_top endpoints" \
  {group_endpoints g_top} {r4/D u/r2/D}

exit_summary
