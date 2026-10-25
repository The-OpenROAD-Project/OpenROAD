# fully_rebuffer with set_opt_config -rebuffer_size_driver: each circuit
# isolates one check that decides whether its driver drv_<name> is resized.
#   base, side_ok, nand_far: controls that are upsized.
#   aoi:       a candidate is timed with its slowest conditional arc.
#   side_crit: a weak top-level input slows its critical sibling load.
#   cap:       the larger input exceeds the input port's max capacitance.
#   nand_near: the other input, weakly driven, becomes critical.
#   tied:      both inputs share one weak net, so both add to its delay.
#   multi:     the input net's second, weak driver sets the extra delay.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits 2000
set m1 [$tech findLayer metal1]
set dont_touch {}
set y 0

proc um { x } {
  return [expr { int($x * 2000) }]
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

# drv_<name> drives a wire of `length` um to ld_<name>.
proc make_circuit { name master inputs length } {
  global block y
  set conns {}
  foreach pin $inputs {
    lappend conns $pin [make_port in_${name}_$pin INPUT 0 $y]
  }
  set net [odb::dbNet_create $block n_$name]
  lappend conns ZN $net
  make_inst drv_$name $master 5 $y $conns
  set out [make_port out_$name OUTPUT [expr { $length + 20 }] $y]
  make_inst ld_$name INV_X1 $length $y [list A $net ZN $out]
  set y [expr { $y + 20 }]
}

# side_<name> is a second load of in_<name>_A.
proc add_sibling { name } {
  global block y
  set in [$block findNet in_${name}_A]
  set out [make_port out_${name}_side OUTPUT 10 [expr { $y - 10 }]]
  make_inst side_$name INV_X1 5 [expr { $y - 10 }] [list A $in ZN $out]
}

foreach name {base cap side_ok side_crit} {
  make_circuit $name INV_X1 {A} 400
}
add_sibling side_ok
add_sibling side_crit
foreach name {nand_far nand_near} {
  make_circuit $name NAND2_X1 {A1 A2} 400
}
make_circuit aoi AOI221_X2 {A B1 B2 C1 C2} 100
make_circuit tied NAND2_X1 {A1} 400
[[$block findInst drv_tied] findITerm A2] connect [$block findNet in_tied_A1]
make_circuit multi INV_X1 {A} 400
set multi_b [odb::dbBTerm_create [$block findNet in_multi_A] in_multi_B]
$multi_b setIoType INPUT
set multi_b_pin [odb::dbBPin_create $multi_b]
# Next to in_multi_A, on the row make_circuit just placed.
set multi_y [expr { $y - 18 }]
odb::dbBox_create $multi_b_pin $m1 0 [um $multi_y] 100 \
  [expr { [um $multi_y] + 100 }]
$multi_b_pin setPlacementStatus LOCKED
foreach net $dont_touch {
  $net setDoNotTouch true
}
ord::design_created

create_clock -period 0.1 -name clk
set_input_delay -clock clk 0 [all_inputs]
set_output_delay -clock clk 0 [all_outputs]
set_driving_cell -lib_cell INV_X1 -pin ZN \
  [get_ports {in_side_ok_A in_side_crit_A}]
set_output_delay -clock clk 0.08 [get_ports out_side_crit_side]
set_max_capacitance 0.003 [get_ports in_cap_A]
set_drive 20 [get_ports {in_nand_far_A2 in_nand_near_A2}]
set_input_delay -clock clk -0.3 [get_ports in_nand_far_A2]
set_input_delay -clock clk -0.07 [get_ports in_nand_near_A2]
set_input_delay -clock clk -0.3 [get_ports {in_aoi_B* in_aoi_C*}]
set_input_delay -clock clk -0.05 [get_ports in_aoi_A]
set_drive 5 [get_ports in_tied_A1]
set_input_delay -clock clk -0.03 [get_ports in_tied_A1]
set_drive 20 [get_ports in_multi_B]
set_input_delay -clock clk -0.06 [get_ports in_multi_B]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

foreach name {base side_ok nand_far aoi side_crit cap nand_near tied multi} {
  puts "drv_$name [[[$block findInst drv_$name] getMaster] getName]"
}

reset_opt_config -rebuffer_size_driver
