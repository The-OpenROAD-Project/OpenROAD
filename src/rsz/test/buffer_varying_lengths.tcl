# Generate wires over a range of lengths to observe buffering trends.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits 2000
# Tech dependent
set m1 [$tech findLayer metal1]
set buf [$db findMaster BUF_X1]
set buf_in [$buf findMTerm A]
set buf_out [$buf findMTerm Z]
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
  set pos [expr { $i * 10 * 5000 }]
  set bp1 [odb::dbBPin_create $bt1]
  set bp3 [odb::dbBPin_create $bt3]
  odb::dbBox_create $bp1 $m1 0 $pos 100 [expr { $pos + 100 }]
  odb::dbBox_create $bp3 $m1 $pos $pos [expr { $pos + 100 }] [expr { $pos + 100 }]
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

proc summarize_solution { } {
  estimate_parasitics -placement
  sta::find_timing
  puts "-------------------------------------------------------------"
  puts " Distance | Buffer area | Path delay | Sequence"
  puts "-------------------------------------------------------------"
  set micron2 [expr [ord::microns_to_dbu 1]**2]
  for { set i 1 } { $i <= 100 } { incr i } {
    set i1o [get_pin i1-$i/Z]
    set i2i [get_pin i2-$i/A]

    set delay [expr [arrival [$i2i vertices]] - [arrival [$i1o vertices]]]

    set area 0.0
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
      [format %11.2f [expr $area / $micron2]] |\
      [format %10.2f [sta::time_sta_ui $delay]] |\
      $sequence"
  }
  puts "-------------------------------------------------------------"
}

create_clock -period 1 -name clk
set_input_delay -clock clk 1.0 [all_inputs]
set_output_delay -clock clk 0.0 [all_outputs]

set_wire_rc -layer metal2
estimate_parasitics -placement

# first, repair design to normalize slews
repair_design
# second, trigger placement buffering
rsz::fully_rebuffer NULL

# characterize solution
puts "\nSolution under tight timing constraint:"
report_check_types
summarize_solution

# now relax timing
set_input_delay -clock clk 0.0 [all_inputs]
# ripup and rebuffer
rsz::fully_rebuffer NULL

# print a second table
puts "\nSolution with relaxed timing:"
report_check_types
summarize_solution
