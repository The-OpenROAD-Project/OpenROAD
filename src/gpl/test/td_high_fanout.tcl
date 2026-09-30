# Timing-driven placement with a net above the default max fanout (50).
# The net is split before parasitics are estimated, and fullyRebuffer
# keeps the resulting buffer tree rather than putting all loads back on
# one Steiner tree.
source helpers.tcl
set test_name td_high_fanout
read_liberty ./library/nangate45/NangateOpenCellLibrary_typical.lib

read_lef ./nangate45.lef
read_def ./simple01-td.def

# Add 56 inverter loads to a flop output net.
set db [ord::get_db]
set block [ord::get_db_block]
set fanout_net [[[$block findInst _569_] findITerm Q] getNet]
set inv [$db findMaster INV_X1]
for { set i 0 } { $i < 56 } { incr i } {
  set load [odb::dbInst_create $block $inv "fanout_load$i"]
  [$load findITerm A] connect $fanout_net
}

create_clock -name core_clock -period 2 clk

set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5

global_placement -timing_driven

set max_pins 0
foreach net [$block getNets] {
  if { [$net getSigType] == "SIGNAL" } {
    set max_pins [expr { max($max_pins, [llength [$net getITerms]]) }]
  }
}
puts "max signal net pins: $max_pins"
