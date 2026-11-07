# fully_rebuffer with set_opt_config -rebuffer_size_driver: a resize is
# checked against the max capacitance of the output being rebuffered.
# drv_ha (ha_2) drives seven inv_8 loads, about 0.13 pF, from SUM only, so
# it may be sized.  ha_1 meets the margined SUM limit (0.8 x 0.1767 pF) but
# not the COUT one (0.8 x 0.1575 pF), so the downsize needs SUM's limit.
source "helpers.tcl"
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef

set db [ord::get_db]
set tech [$db getTech]
set block [odb::dbBlock_create [odb::dbChip_create $db $tech] top]
set dbu [$tech getDbUnitsPerMicron]
$block setDefUnits $dbu
set m1 [$tech findLayer met1]
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
  odb::dbBox_create $bpin $m1 [um $x] [um $y] [expr { [um $x] + 100 }] \
    [expr { [um $y] + 100 }]
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

set sum [odb::dbNet_create $block n_sum]
make_inst drv_ha sky130_fd_sc_hd__ha_2 5 0 [list A [make_port in_A INPUT 0 0] \
  B [make_port in_B INPUT 0 3] SUM $sum]
for { set i 0 } { $i < 7 } { incr i } {
  set x [expr { 20 + 5 * $i }]
  make_inst ld_$i sky130_fd_sc_hd__inv_8 $x 0 \
    [list A $sum Y [make_port out_$i OUTPUT $x 10]]
}
foreach net $dont_touch {
  $net setDoNotTouch true
}
ord::design_created

create_clock -period 2 -name clk
set_input_delay -clock clk 0 [all_inputs]
set_output_delay -clock clk -1.5 [all_outputs]
source sky130hd/sky130hd.rc
set_wire_rc -layer met2
estimate_parasitics -placement

set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

puts "drv_ha [[[$block findInst drv_ha] getMaster] getName]"

reset_opt_config -rebuffer_size_driver
