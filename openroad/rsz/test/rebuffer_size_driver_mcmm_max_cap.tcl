# fully_rebuffer with set_opt_config -rebuffer_size_driver keeps the tree
# for an upsized driver within the max_capacitance of every scene, not only
# of the command scene.  Scene "active" uses Nangate45 with AOI21_X4/ZN
# limited to 60 fF and a 0.05 ns clock, so its paths are the worst; scene
# "loose" uses the unchanged Nangate45 (101 fF) with a 1.0 ns clock and is
# the command scene.  AOI21_X1 x drives a 66 fF critical port and a 22 fF
# port.  Checked against the command scene alone, the tree planned for
# AOI21_X4 drives the critical port directly and violates the 60 fF limit.
source "helpers.tcl"

# Nangate45 renamed active_cap, with AOI21_X4/ZN max_capacitance 60 fF.
set lib_file [open Nangate45/Nangate45_typ.lib r]
set lib [read $lib_file]
close $lib_file
set lib [string map \
  {"library (NangateOpenCellLibrary)" "library (active_cap)"} $lib]
set cell_at [string first "cell (AOI21_X4)" $lib]
set cap_at [string first "max_capacitance" $lib $cell_at]
set cap_end [string first ";" $lib $cap_at]
set lib [string replace $lib $cap_at $cap_end "max_capacitance : 60;"]
set active_lib [make_result_file active_cap.lib]
set lib_file [open $active_lib w]
puts -nonewline $lib_file $lib
close $lib_file

read_liberty $active_lib
read_liberty Nangate45/Nangate45_typ.lib
set_mode strict_m
set_mode loose_m
define_scene active -mode strict_m -liberty $active_lib
define_scene loose -mode loose_m -liberty Nangate45/Nangate45_typ.lib
set_scene loose
read_lef Nangate45/Nangate45.lef

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits [$tech getDbUnitsPerMicron]
set pin_layer [$tech findLayer metal1]

# Top-level port on net at (x, y) um.
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

# Each scene with its own clock period and the same loads.
foreach { scene period } { active 0.05 loose 1.0 } {
  set_scene $scene
  create_clock -period $period -name clk
  set_input_delay -clock clk 0 [all_inputs]
  set_output_delay -clock clk 0 [get_ports crit]
  set_output_delay -clock clk -0.02 [get_ports slow]
  set_load 66 [get_ports crit]
  set_load 22 [get_ports slow]
}
set_scene loose
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
report_check_types -scenes active -max_capacitance -digits 3
report_check_types -scenes loose -max_capacitance -digits 3
report_worst_slack -max -digits 3
report_tns -digits 3

reset_opt_config -rebuffer_size_driver
