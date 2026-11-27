# fully_rebuffer with set_opt_config -rebuffer_size_driver holds a shared
# input net with the cap growth guard only until the next timing refresh.
# NAND2_X1 g_a and g_b load the weakly driven, failing net "in" on A1.  g_a's
# late A2 comes through two inverters, so it is sized two levels before g_b.
# Upsizing g_a grows the pin load of "in" past the limit; the level refresh
# then times that resize, and the side-load check sees it.  g_b may upsize
# too, which also helps "in", whose worst path runs through g_b.  Keeping the
# growth record for the whole pass held g_b at NAND2_X1.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits [$tech getDbUnitsPerMicron]
set pin_layer [$tech findLayer metal1]
# Net connected to a placed top-level port at (x, y) um.
proc make_port { name io x y } {
  global block pin_layer
  set net [odb::dbNet_create $block $name]
  set bterm [odb::dbBTerm_create $net $name]
  $bterm setIoType $io
  set bpin [odb::dbBPin_create $bterm]
  set x [ord::microns_to_dbu $x]
  set y [ord::microns_to_dbu $y]
  odb::dbBox_create $bpin $pin_layer $x $y [expr { $x + 100 }] [expr { $y + 100 }]
  $bpin setPlacementStatus LOCKED
  return $net
}
# Placed instance of master at (x, y) um with {term net} connections.
proc make_inst { master name conns x y } {
  global block db
  set inst [odb::dbInst_create $block [$db findMaster $master] $name]
  foreach { term net } $conns { [$inst findITerm $term] connect $net }
  $inst setLocation [ord::microns_to_dbu $x] [ord::microns_to_dbu $y]
  $inst setPlacementStatus PLACED
  return $inst
}
# in -> g_a, g_b; b -> c1 -> c2 -> g_a/A2; c -> g_b/A2.  Each g drives a
# dont_touch INV_X16 load next to its output port.
set in [make_port in INPUT 0 0]
set b [make_port b INPUT 0 4]
set c [make_port c INPUT 0 8]
set b1 [odb::dbNet_create $block b1]
set bb [odb::dbNet_create $block bb]
set i1 [make_inst INV_X4 c1 [list A $b ZN $b1] 1 4]
set i2 [make_inst INV_X4 c2 [list A $b1 ZN $bb] 2 4]
$i1 setDoNotTouch true
$i2 setDoNotTouch true
set za [odb::dbNet_create $block za]
set zb [odb::dbNet_create $block zb]
make_inst NAND2_X1 g_a [list A1 $in A2 $bb ZN $za] 4 4
make_inst NAND2_X1 g_b [list A1 $in A2 $c ZN $zb] 4 8
set oa [make_port out_a OUTPUT 24 4]
set ob [make_port out_b OUTPUT 24 8]
set la [make_inst INV_X16 l_a [list A $za ZN $oa] 20 4]
set lb [make_inst INV_X16 l_b [list A $zb ZN $ob] 20 8]
$la setDoNotTouch true
$lb setDoNotTouch true
ord::design_created
# The A2 paths of g_a and g_b and the A1 paths through "in" fail.
create_clock -period 0.4 -name clk
set_input_delay -clock clk 0.30 [get_ports in]
set_input_delay -clock clk 0.40 [get_ports b]
set_input_delay -clock clk 0.41 [get_ports c]
set_output_delay -clock clk 0 [all_outputs]
set_drive 5 [get_ports in]
set_dont_use NAND2_X4
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
# Masters of g_a and g_b, and the slacks of "in" and both A2 paths.
proc report_state { } {
  puts "g_a [get_property [get_cells g_a] ref_name] g_b [get_property [get_cells g_b] ref_name]"
  puts [format "slack in %.4f g_a/A2 %.4f g_b/A2 %.4f" \
    [get_property [get_ports in] slack_max] \
    [get_property [get_pins g_a/A2] slack_max] \
    [get_property [get_pins g_b/A2] slack_max]]
  report_worst_slack -max -digits 4
  report_tns -digits 4
}
report_state
set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

estimate_parasitics -placement
report_state

reset_opt_config -rebuffer_size_driver
