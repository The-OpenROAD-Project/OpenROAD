# Verify repair_design repeater sizing under set_opt_config sizing limits:
# 1. slew_net: locked BUF_X1 driving a 100 fF port load (cap-clean for BUF_X2's
#    121.15 fF max_cap, but BUF_X2's 0.11 ns slew violates the 50%-margined
#    0.10 ns max_transition) expands past sizing limits to BUF_X4.
# 2. cap_net: locked BUF_X1 driving a 200 fF port load (exceeding BUF_X2's
#    121.15 fF max_cap and BUF_X4's 0.11 ns slew) expands past sizing limits to
#    BUF_X8.
# 3. wire_net: locked BUF_X1 driving a 1000 um wire to BUF_X1 stays within the
#    1.5X sizing limit (BUF_X2) because each buffered segment is DRC-clean.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits 2000

set m1 [$tech findLayer metal1]
set buf_x1 [$db findMaster BUF_X1]

proc make_locked_port { block layer net name io_type x_um y_um } {
  set dbu [$block getDbUnitsPerMicron]
  set x [expr { $x_um * $dbu }]
  set y [expr { $y_um * $dbu }]
  set bt [odb::dbBTerm_create $net $name]
  $bt setIoType $io_type
  $bt connect $net
  set bp [odb::dbBPin_create $bt]
  odb::dbBox_create $bp $layer $x $y [expr { $x + 100 }] [expr { $y + 100 }]
  $bp setPlacementStatus LOCKED
  return $bt
}

proc make_locked_buf { block master name in_net out_net x_um y_um } {
  set dbu [$block getDbUnitsPerMicron]
  set inst [odb::dbInst_create $block $master $name]
  [$inst findITerm A] connect $in_net
  [$inst findITerm Z] connect $out_net
  $inst setLocation [expr { $x_um * $dbu }] [expr { $y_um * $dbu }]
  $inst setPlacementStatus LOCKED
  return $inst
}

# Net 1: Slew-DRC expansion (100 fF is cap-clean for BUF_X2, but > 0.10 ns slew).
set in_slew_net [odb::dbNet_create $block in_slew_net]
set slew_net [odb::dbNet_create $block slew_net]
make_locked_port $block $m1 $in_slew_net in_slew INPUT 0 0
make_locked_port $block $m1 $slew_net out_slew OUTPUT 20 0
make_locked_buf $block $buf_x1 slew_drv $in_slew_net $slew_net 0 0
$in_slew_net setDoNotTouch true

# Net 2: Heavy 200 fF capacitive port load (exceeds BUF_X2 max_cap + slew).
set in_cap_net [odb::dbNet_create $block in_cap_net]
set cap_net [odb::dbNet_create $block cap_net]
make_locked_port $block $m1 $in_cap_net in_cap INPUT 0 20
make_locked_port $block $m1 $cap_net out_cap OUTPUT 20 20
make_locked_buf $block $buf_x1 cap_drv $in_cap_net $cap_net 0 20
$in_cap_net setDoNotTouch true

# Net 3: Moderate 1000 um wire to BUF_X1 (DRC-clean within sizing limits).
set in_wire_net [odb::dbNet_create $block in_wire_net]
set wire_net [odb::dbNet_create $block wire_net]
set out_wire_net [odb::dbNet_create $block out_wire_net]
make_locked_port $block $m1 $in_wire_net in_wire INPUT 0 40
make_locked_port $block $m1 $out_wire_net out_wire OUTPUT 1000 40
make_locked_buf $block $buf_x1 wire_drv $in_wire_net $wire_net 0 40
make_locked_buf $block $buf_x1 wire_load $wire_net $out_wire_net 1000 40
$in_wire_net setDoNotTouch true
$out_wire_net setDoNotTouch true

ord::design_created

create_clock -period 1.0 -name clk
set_input_delay -clock clk 0.0 [all_inputs]
set_output_delay -clock clk 0.0 [all_outputs]
set_load 100 [get_ports out_slew]
set_load 200 [get_ports out_cap]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

puts "Violations before repair_design:"
report_check_types -max_slew -max_capacitance -violators

# Restrict sizing to 1.5X area / 2.5X leakage (caps BUF_X1 soft window at BUF_X2).
set_opt_config -limit_sizing_area 1.5 -limit_sizing_leakage 2.5
repair_design -slew_margin 50

estimate_parasitics -placement
puts "\nChecks after repair_design:"
report_check_types -max_slew -max_capacitance
puts "Remaining violators after repair_design:"
report_check_types -max_slew -max_capacitance -violators

proc report_inserted_repeaters { label drvr_pin_name endpoint_inst_names } {
  set drvr_pin [get_pin $drvr_pin_name]
  set repeaters {}
  foreach cell [get_fanout -only_cells -from $drvr_pin] {
    set cell_name [get_full_name $cell]
    if {
      $cell != [$drvr_pin instance]
      && [lsearch -exact $endpoint_inst_names $cell_name] < 0
      && [$cell liberty_cell] != "NULL"
    } {
      set master [sta::sta_to_db_master [$cell liberty_cell]]
      lappend repeaters [$master getName]
    }
  }
  puts "$label repeaters: [join [lsort $repeaters] { }]"
}

puts "\nInserted repeaters by net:"
report_inserted_repeaters "slew_net" "slew_drv/Z" {}
report_inserted_repeaters "cap_net" "cap_drv/Z" {}
report_inserted_repeaters "wire_net" "wire_drv/Z" {wire_load}
