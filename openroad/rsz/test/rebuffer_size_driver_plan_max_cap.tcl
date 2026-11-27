# fully_rebuffer with set_opt_config -rebuffer_size_driver: the tree planned
# for the upsized driver must fit that cell's Liberty max_capacitance.
# AOI21_X1 drives a 66 fF critical port and a 22 fF port with 20 ps of
# margin.  With only BUF_X1/BUF_X2 usable, the fastest tree for AOI21_X4
# drives both ports directly (~90 fF), over its max_capacitance less the
# margin (~81 fF), so the final choice would reject it.  Planning must
# instead buffer only the relaxed port and keep the critical one direct.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits [$tech getDbUnitsPerMicron]
set pin_layer [$tech findLayer metal1]

# Placed top-level port on net.
proc make_port { net name io x y } {
  global block pin_layer
  set bterm [odb::dbBTerm_create $net $name]
  $bterm setIoType $io
  set bpin [odb::dbBPin_create $bterm]
  set x [ord::microns_to_dbu $x]
  set y [ord::microns_to_dbu $y]
  odb::dbBox_create $bpin $pin_layer $x $y [expr { $x + 100 }] [expr { $y + 100 }]
  $bpin setPlacementStatus LOCKED
}

# AOI21_X1 x, all inputs on port in, drives ports crit and slow.
set in [odb::dbNet_create $block in]
set n [odb::dbNet_create $block n]
make_port $in in INPUT 0 0
set x [odb::dbInst_create $block [$db findMaster AOI21_X1] x]
foreach term { A B1 B2 } {
  [$x findITerm $term] connect $in
}
[$x findITerm ZN] connect $n
$x setLocation [ord::microns_to_dbu 5] 0
$x setPlacementStatus PLACED
make_port $n crit OUTPUT 20 0
make_port $n slow OUTPUT 25 0
ord::design_created

create_clock -period 0.05 -name clk
set_input_delay -clock clk 0 [all_inputs]
set_output_delay -clock clk 0 [get_ports crit]
set_output_delay -clock clk -0.02 [get_ports slow]
set_load 66 [get_ports crit]
set_load 22 [get_ports slow]
# Only BUF_X1 and BUF_X2 may be inserted.
set_dont_use { BUF_X4 BUF_X8 BUF_X16 BUF_X32 }
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# x's master and the sinks it drives directly (ports, or buffer master/pin).
proc report_x_net { } {
  set block [ord::get_db_block]
  set x [$block findInst x]
  set net [[$x findITerm ZN] getNet]
  set sinks {}
  foreach bterm [$net getBTerms] {
    lappend sinks [$bterm getName]
  }
  foreach iterm [$net getITerms] {
    set inst [$iterm getInst]
    if { [$inst getName] ne "x" } {
      lappend sinks [[$inst getMaster] getName]/[[$iterm getMTerm] getName]
    }
  }
  puts "x: [[$x getMaster] getName] drives: [lsort $sinks]"
}

report_x_net
report_worst_slack -max -digits 3
report_tns -digits 3

set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

estimate_parasitics -placement
report_x_net
report_worst_slack -max -digits 3
report_tns -digits 3

reset_opt_config -rebuffer_size_driver
