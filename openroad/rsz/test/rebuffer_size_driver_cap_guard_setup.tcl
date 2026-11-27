# fully_rebuffer with set_opt_config -rebuffer_size_driver judges a capped
# shared net on setup (max) capacitance only.  NAND2_X1 g0..g2 share the
# failing port "in" (g1 on A2, the others on A1) and take their failing
# paths from their own late ports.  NAND2_X4 is dont_use, so the only upsize
# is NAND2_X2.  Inputs are 1 fF in the late (max) library except the rise
# capacitance of NAND2_X2/A1 (2 fF), so upsizing g0 or g2 caps "in" on rise.
# The early (min) library raises NAND2_X2/A2 to 2 fF.  g1 may still take
# NAND2_X2, which adds no setup capacitance to "in"; counting the early
# capacitance kept it at NAND2_X1.
source "helpers.tcl"

# Copy of Nangate45 named `name` with each {cell pin cap fall} input pin
# set to `cap` fF (rise) and `fall` fF.
proc nangate45_copy { name pins } {
  set lib_file [open Nangate45/Nangate45_typ.lib r]
  set lib [read $lib_file]
  close $lib_file
  set lib [string map [list "library (NangateOpenCellLibrary)" "library ($name)"] $lib]
  foreach { cell pin cap fall } $pins {
    set at [string first "pin ($pin)" $lib [string first "cell ($cell)" $lib]]
    foreach { key value } [list capacitance $cap fall_capacitance $fall rise_capacitance $cap] {
      set key_at [string first "\t$key" $lib $at]
      set end [string first ";" $lib $key_at]
      set lib [string replace $lib $key_at $end "\t$key : $value;"]
    }
  }
  set path [make_result_file $name.lib]
  set lib_file [open $path w]
  puts -nonewline $lib_file $lib
  close $lib_file
  return $path
}
set same { NAND2_X1 A1 1 1 NAND2_X1 A2 1 1 NAND2_X2 A1 2 1 }
set late_lib [nangate45_copy late_lib [concat $same { NAND2_X2 A2 1 1 }]]
set early_lib [nangate45_copy early_lib [concat $same { NAND2_X2 A2 2 2 }]]
read_liberty $late_lib
read_liberty $early_lib
define_scene s -liberty_max $late_lib -liberty_min $early_lib
set_scene s
read_lef Nangate45/Nangate45.lef
set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits [$tech getDbUnitsPerMicron]
set pin_layer [$tech findLayer metal1]
proc make_port { name io x y } {
  global block pin_layer
  set net [odb::dbNet_create $block $name]
  set bterm [odb::dbBTerm_create $net $name]
  $bterm setIoType $io
  set bpin [odb::dbBPin_create $bterm]
  odb::dbBox_create $bpin $pin_layer $x $y [expr { $x + 100 }] [expr { $y + 100 }]
  $bpin setPlacementStatus LOCKED
  return $net
}
proc make_inst { master name conns x y } {
  global block db
  set inst [odb::dbInst_create $block [$db findMaster $master] $name]
  foreach { term net } $conns { [$inst findITerm $term] connect $net }
  $inst setLocation $x $y
  $inst setPlacementStatus PLACED
  return $inst
}
set dist 20
set in [make_port in INPUT 0 0]
for { set i 0 } { $i < 3 } { incr i } {
  set y [ord::microns_to_dbu [expr { 3 * $i }]]
  set x [ord::microns_to_dbu $dist]
  set a [make_port a$i INPUT [expr { $x - [ord::microns_to_dbu 2] }] $y]
  set z [odb::dbNet_create $block z$i]
  set out [make_port out$i OUTPUT [expr { $x + [ord::microns_to_dbu 20] }] $y]
  set anet [expr { $i == 1 ? $a : $in }]
  set bnet [expr { $i == 1 ? $in : $a }]
  make_inst NAND2_X1 g$i [list A1 $anet A2 $bnet ZN $z] $x $y
  set load [make_inst INV_X16 l$i [list A $z ZN $out] [expr { $x + [ord::microns_to_dbu 16] }] $y]
  $load setDoNotTouch true
}
ord::design_created
create_clock -period 0.4 -name clk
set_input_delay -clock clk 0.35 [get_ports in]
set_input_delay -clock clk 0.48 [get_ports a*]
set_output_delay -clock clk 0 [all_outputs]
set_drive 0.35 [get_ports in]
set_drive 0.35 [get_ports a*]
set_dont_use NAND2_X4
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
proc report_state { } {
  set masters {}
  foreach name { g0 g1 g2 } { lappend masters "$name [get_property [get_cells $name] ref_name]" }
  puts [join $masters ", "]
  puts [format "in slack %.4f" [get_property [get_ports in] slack_max]]
  foreach i { 0 1 2 } {
    puts -nonewline [format "a$i %.4f " [get_property [get_ports a$i] slack_max]]
  }
  puts ""
  report_worst_slack -max -digits 4
  report_tns -digits 4
}
report_state
report_net -digits 4 in
set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL
estimate_parasitics -placement
report_state
report_net -digits 4 in

reset_opt_config -rebuffer_size_driver
