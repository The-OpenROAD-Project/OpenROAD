# Generate wires over a range of lengths to observe buffering trends.
# buffer_varying_lengths_size_driver.tcl runs the same flow with driver
# sizing enabled during rebuffering, and the *_asap7* tests set
# buffer_varying_lengths_pdk (see buffer_varying_lengths_pdk.tcl).
# rebuffer_reference/ sources it with buffer_varying_lengths_tight_only set
# to stop after the tight-timing step.
source "helpers.tcl"
source "buffer_varying_lengths_pdk.tcl"
if { ![info exists buffer_varying_lengths_size_driver] } {
  set buffer_varying_lengths_size_driver 0
}
if { ![info exists buffer_varying_lengths_tight_only] } {
  set buffer_varying_lengths_tight_only 0
}
bvl_read_pdk

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits [$tech getDbUnitsPerMicron]
set pin_layer [$tech findLayer $bvl_pin_layer]
set buf [$db findMaster $bvl_buffer]
set buf_in [$buf findMTerm $bvl_buffer_in]
set buf_out [$buf findMTerm $bvl_buffer_out]
set step [expr { round($bvl_step_um * [$tech getDbUnitsPerMicron]) }]
for { set i 1 } { $i <= 100 } { incr i } {
  set n1 [odb::dbNet_create $block n1-$i]
  set n2 [odb::dbNet_create $block n2-$i]
  set n3 [odb::dbNet_create $block n3-$i]
  set bt1 [odb::dbBTerm_create $n1 p1-$i]
  set bt3 [odb::dbBTerm_create $n3 p3-$i]
  $bt1 setIoType INPUT
  $bt3 setIoType OUTPUT
  $bt1 connect $n1
  $bt3 connect $n3
  set pos [expr { $i * $step }]
  set bp1 [odb::dbBPin_create $bt1]
  set bp3 [odb::dbBPin_create $bt3]
  odb::dbBox_create $bp1 $pin_layer 0 $pos 100 [expr { $pos + 100 }]
  odb::dbBox_create $bp3 $pin_layer $pos $pos [expr { $pos + 100 }] [expr { $pos + 100 }]
  $bp1 setPlacementStatus LOCKED
  $bp3 setPlacementStatus LOCKED
  set i1 [odb::dbInst_create $block $buf i1-$i]
  [$i1 getITerm $buf_in] connect $n1
  [$i1 getITerm $buf_out] connect $n2
  $n1 setDoNotTouch true
  $i1 setLocation 0 $pos
  $i1 setPlacementStatus LOCKED
  set i2 [odb::dbInst_create $block $buf i2-$i]
  [$i2 getITerm $buf_in] connect $n2
  [$i2 getITerm $buf_out] connect $n3
  $n3 setDoNotTouch true
  $i2 setLocation $pos $pos
  $i2 setPlacementStatus LOCKED
}
ord::design_created

proc arrival { vertex } {
  return [[sta::vertex_worst_arrival_path $vertex max] arrival]
}

# Path delay is measured to the endpoint (i2/Z) from the start of the
# input external delay, so it includes the port driving cell delay into
# the driver and the load delay that depends on the delivered slew.
proc summarize_solution { input_delay } {
  global bvl_buffer_in bvl_buffer_out
  estimate_parasitics -placement
  sta::find_timing
  puts "----------------------------------------------------------------------"
  puts " Distance | Driver  | Area  | Path delay | Sequence"
  puts "----------------------------------------------------------------------"
  set micron2 [expr [ord::microns_to_dbu 1]**2]
  for { set i 1 } { $i <= 100 } { incr i } {
    set i1o [get_pin i1-$i/$bvl_buffer_out]
    set i2i [get_pin i2-$i/$bvl_buffer_in]
    set i2o [get_pin i2-$i/$bvl_buffer_out]

    set delay [expr [arrival [$i2o vertices]] - [sta::time_ui_sta $input_delay]]

    set driver [[$i1o instance] liberty_cell]
    set driver_master [sta::sta_to_db_master $driver]
    set area [expr { double([$driver_master getArea]) }]
    set sequence ""
    foreach buffer [get_fanout -only_cells -from $i1o] {
      if {
        $buffer != [$i1o instance] &&
        $buffer != [$i2i instance] &&
        [$buffer liberty_cell] != "NULL"
      } {
        set master [sta::sta_to_db_master [$buffer liberty_cell]]
        set area [expr $area + [$master getArea]]
        set sequence "$sequence [$master getName]"
      }
    }

    lassign [[[sta::sta_to_db_pin $i1o] getInst] getLocation] x1 y1
    lassign [[[sta::sta_to_db_pin $i2i] getInst] getLocation] x2 y2
    set distance [expr abs($x1 - $x2) + abs($y1 - $y2)]

    puts " [format %8.1f [ord::dbu_to_microns $distance]] |\
      [format %-7s [$driver_master getName]] |\
      [format %5.2f [expr $area / $micron2]] |\
      [format %10.3f [sta::time_sta_ui $delay]] |\
      $sequence"
  }
  puts "----------------------------------------------------------------------"
}

create_clock -period $bvl_period -name clk
set_input_delay -clock clk $bvl_period [all_inputs]
set_output_delay -clock clk 0.0 [all_outputs]
set_driving_cell -lib_cell $bvl_buffer -pin $bvl_buffer_out [all_inputs]

bvl_set_wire_rc
estimate_parasitics -placement

# first, repair design to normalize slews
repair_design
if { $buffer_varying_lengths_size_driver } {
  set_opt_config -rebuffer_size_driver true
  # Rebuffering does not resize fixed instances, so unlock the drivers.
  for { set i 1 } { $i <= 100 } { incr i } {
    set driver [$block findInst i1-$i]
    $driver setPlacementStatus PLACED
    set driver_location($i) [$driver getLocation]
  }
}

# Resizing an unlocked driver must not move it.
proc check_drivers_unmoved { } {
  global block buffer_varying_lengths_size_driver driver_location
  if { !$buffer_varying_lengths_size_driver } {
    return
  }
  for { set i 1 } { $i <= 100 } { incr i } {
    set location [[$block findInst i1-$i] getLocation]
    if { $location != $driver_location($i) } {
      error "driver i1-$i moved from $driver_location($i) to $location"
    }
  }
}

# second, trigger placement buffering
rsz::fully_rebuffer NULL
check_drivers_unmoved

# characterize solution
puts "\nSolution under tight timing constraint:"
report_check_types
summarize_solution $bvl_period
if { $buffer_varying_lengths_tight_only } {
  return
}

# now relax timing
set_input_delay -clock clk 0.0 [all_inputs]
# ripup and rebuffer
rsz::fully_rebuffer NULL
check_drivers_unmoved

# print a second table
puts "\nSolution with relaxed timing:"
report_check_types
summarize_solution 0.0
