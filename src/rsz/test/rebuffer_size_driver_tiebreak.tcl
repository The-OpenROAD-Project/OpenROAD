# fully_rebuffer with set_opt_config -rebuffer_size_driver: an equal-area
# swap to a faster cell is taken only on a net that misses timing.  INVxp67
# and INVx1 have the same area, and both drivers start as INVx2.  drv_crit
# becomes INVx1, while drv_relaxed, which meets timing, becomes INVxp67
# rather than the equal-area INVx1.  drv_ha, a critical half adder with
# both outputs connected, keeps its cell although its L and SL variants have
# the same area.
source "helpers.tcl"
source asap7/asap7.vars
foreach vt {RVT LVT SLVT} {
  read_liberty asap7/asap7sc7p5t_INVBUF_${vt}_FF_nldm_220122.lib.gz
  read_liberty asap7/asap7sc7p5t_SIMPLE_${vt}_FF_nldm_211120.lib.gz
}
read_lef asap7/asap7_tech_1x_201209.lef
foreach vt {R L SL} {
  read_lef asap7/asap7sc7p5t_28_${vt}_1x_220121a.lef
}

set db [ord::get_db]
set tech [$db getTech]
set block [odb::dbBlock_create [odb::dbChip_create $db $tech] top]
set dbu [$tech getDbUnitsPerMicron]
$block setDefUnits $dbu
set m1 [$tech findLayer M1]
set dont_touch {}

proc um { x } {
  global dbu
  return [expr { int($x * $dbu) }]
}

proc make_port { name dir x y } {
  global block m1 dont_touch
  set net [odb::dbNet_create $block $name]
  set bterm [odb::dbBTerm_create $net $name]
  $bterm setIoType $dir
  set bpin [odb::dbBPin_create $bterm]
  odb::dbBox_create $bpin $m1 [um $x] [um $y] [expr { [um $x] + 20 }] \
    [expr { [um $y] + 20 }]
  $bpin setPlacementStatus LOCKED
  lappend dont_touch $net
  return $net
}

proc make_inst { name master x y conns } {
  global block db
  set inst [odb::dbInst_create $block [$db findMaster $master] $name]
  $inst setLocation [um $x] [um $y]
  $inst setPlacementStatus PLACED
  dict for {pin net} $conns {
    [$inst findITerm $pin] connect $net
  }
}

# in_<name> -> drv_<name> (INVx2) -> 10 um wire -> ld_<name> -> out_<name>
set y 0
foreach name {crit relaxed} {
  set in [make_port in_$name INPUT 0 $y]
  set out [make_port out_$name OUTPUT 15 $y]
  set net [odb::dbNet_create $block n_$name]
  make_inst drv_$name INVx2_ASAP7_75t_R 1 $y [list A $in Y $net]
  make_inst ld_$name INVx1_ASAP7_75t_R 10 $y [list A $net Y $out]
  set y [expr { $y + 5 }]
}
# in_ha_A/B -> drv_ha (HAxp5) -> SN: 10 um wire -> ld_ha -> out_ha; CON -> out_ha_co
set ha_sn [odb::dbNet_create $block n_ha]
make_inst drv_ha HAxp5_ASAP7_75t_R 1 $y [list A [make_port in_ha_A INPUT 0 $y] \
  B [make_port in_ha_B INPUT 0 [expr { $y + 1 }]] SN $ha_sn \
  CON [make_port out_ha_co OUTPUT 5 [expr { $y + 2 }]]]
make_inst ld_ha INVx1_ASAP7_75t_R 10 $y \
  [list A $ha_sn Y [make_port out_ha OUTPUT 15 $y]]
foreach net $dont_touch {
  $net setDoNotTouch true
}
ord::design_created

create_clock -period 25 -name clk
set_input_delay -clock clk 0 [all_inputs]
set_output_delay -clock clk 0 [get_ports {out_crit out_ha out_ha_co}]
set_output_delay -clock clk -30 [get_ports out_relaxed]
source asap7/setRC.tcl
estimate_parasitics -placement

set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

foreach name {crit relaxed ha} {
  puts "drv_$name [[[$block findInst drv_$name] getMaster] getName]"
}

reset_opt_config -rebuffer_size_driver
