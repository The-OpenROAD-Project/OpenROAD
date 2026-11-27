# fully_rebuffer with set_opt_config -rebuffer_size_driver charges no area
# upsize hurdle to a driver that was on the worst path at pass start.
# Inverter d drives net "in", which feeds g0..g2, each driving a 20 fF output
# port.  The g output nets are rebuffered first and lift d's slack out of the
# critical band, which stays at the pass-start worst slack.  Judged on that
# alone, d pays the hurdle and takes INVx2 (WNS -18.2 ps); counted as
# critical from its pass-start slack, it takes INVx3 (WNS -16.3 ps).
source "helpers.tcl"
foreach vt { RVT LVT SLVT } {
  read_liberty asap7/asap7sc7p5t_INVBUF_${vt}_FF_nldm_220122.lib.gz
}
read_lef asap7/asap7_tech_1x_201209.lef
foreach vt { R L SL } {
  read_lef asap7/asap7sc7p5t_28_${vt}_1x_220121a.lef
}

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits [$tech getDbUnitsPerMicron]
set pin_layer [$tech findLayer M1]

# Top-level port on net at (x, y) um.
proc make_port { net name io x y } {
  global pin_layer
  set bterm [odb::dbBTerm_create $net $name]
  $bterm setIoType $io
  set bpin [odb::dbBPin_create $bterm]
  set x [ord::microns_to_dbu $x]
  set y [ord::microns_to_dbu $y]
  odb::dbBox_create $bpin $pin_layer $x $y [expr { $x + 100 }] [expr { $y + 100 }]
  $bpin setPlacementStatus LOCKED
}

# Placed instance of master at (x, y) um with {term net} connections.
proc make_inst { master name conns x y } {
  global block db
  set inst [odb::dbInst_create $block [$db findMaster $master] $name]
  foreach { term net } $conns {
    [$inst findITerm $term] connect $net
  }
  $inst setLocation [ord::microns_to_dbu $x] [ord::microns_to_dbu $y]
  $inst setPlacementStatus PLACED
}

# src -> d -> in -> g0..g2 -> out0..out2
set src [odb::dbNet_create $block src]
set in [odb::dbNet_create $block in]
make_port $src src INPUT 0 0
make_inst INVx1_ASAP7_75t_R d [list A $src Y $in] 1 0
for { set i 0 } { $i < 3 } { incr i } {
  set y [expr { 2 * $i }]
  set o [odb::dbNet_create $block o$i]
  make_inst INVx1_ASAP7_75t_R g$i [list A $in Y $o] 3 $y
  make_port $o out$i OUTPUT 6 $y
}
ord::design_created

set_dont_use {*xp*_ASAP7*}
create_clock -period 15 -name clk
set_input_delay -clock clk 0 [all_inputs]
set_output_delay -clock clk 0 [all_outputs]
set_load 20 [all_outputs]
source asap7/setRC.tcl
estimate_parasitics -placement

# Masters of d and g0..g2.
proc report_state { } {
  set masters {}
  foreach name { d g0 g1 g2 } {
    lappend masters "$name [get_property [get_cells $name] ref_name]"
  }
  puts [join $masters ", "]
  report_worst_slack -max -digits 2
  report_tns -digits 2
}

report_state

set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

estimate_parasitics -placement
report_state

reset_opt_config -rebuffer_size_driver
