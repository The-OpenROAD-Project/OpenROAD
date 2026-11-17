# Shared setup for the buffer_varying_lengths reference search.
#
# Each candidate is an independent path built like one path of
# buffer_varying_lengths.tcl: input port -> driver -> buffer chain -> BUF_X1
# load -> output port, with the load placed 25 um * i from the driver.
# Paths are stacked vertically so many candidates are timed in one STA run.
#
# Run from src/rsz/test so the Nangate45 paths resolve.

namespace eval ref {
variable cells {BUF_X1 BUF_X2 BUF_X4 BUF_X8 BUF_X16 BUF_X32}
variable npaths 0
variable input_delay 1.0
}

proc ref::init { } {
  variable block
  variable m1
  variable masters
  variable cells
  read_liberty Nangate45/Nangate45_typ.lib
  read_lef Nangate45/Nangate45.lef
  set db [ord::get_db]
  set tech [$db getTech]
  set chip [odb::dbChip_create $db $tech]
  set block [odb::dbBlock_create $chip top]
  $block setDefUnits 2000
  set m1 [$tech findLayer metal1]
  foreach cell $cells {
    set masters($cell) [$db findMaster $cell]
  }
}

# Same constraints as the tight-timing step of buffer_varying_lengths.tcl.
proc ref::constrain { } {
  variable input_delay
  create_clock -period 1 -name clk
  set_input_delay -clock clk $input_delay [all_inputs]
  set_output_delay -clock clk 0.0 [all_outputs]
  set_driving_cell -lib_cell BUF_X1 -pin Z [all_inputs]
  set_wire_rc -layer metal2
}

proc ref::length_dbu { i } {
  return [expr { $i * 50000 }]
}

# Evenly spaced x positions for k buffers on path length i.
proc ref::uniform_xs { i k } {
  set length [ref::length_dbu $i]
  set xs {}
  for { set j 1 } { $j <= $k } { incr j } {
    lappend xs [expr { int($length * $j / ($k + 1.0)) }]
  }
  return $xs
}

proc ref::make_bterm { net name io_type x y } {
  variable m1
  set bterm [odb::dbBTerm_create $net $name]
  $bterm setIoType $io_type
  set bpin [odb::dbBPin_create $bterm]
  odb::dbBox_create $bpin $m1 $x $y [expr { $x + 100 }] [expr { $y + 100 }]
  $bpin setPlacementStatus LOCKED
}

proc ref::make_inst { cell name x y } {
  variable block
  variable masters
  set inst [odb::dbInst_create $block $masters($cell) $name]
  $inst setLocation $x $y
  $inst setPlacementStatus PLACED
  return $inst
}

# Build one candidate path; returns its index.
proc ref::make_path { i driver seq xs } {
  variable block
  variable npaths
  variable path
  set p $npaths
  incr npaths
  set length [ref::length_dbu $i]
  set y [expr { $p * 4000 }]

  set in_net [odb::dbNet_create $block in_$p]
  ref::make_bterm $in_net in_$p INPUT 0 $y
  set drvr [ref::make_inst $driver drvr_$p 0 $y]
  [$drvr findITerm A] connect $in_net
  set prev [$drvr findITerm Z]

  set bufs {}
  set k 0
  foreach cell $seq x $xs {
    set buf [ref::make_inst $cell buf_${p}_$k $x $y]
    set net [odb::dbNet_create $block wire_${p}_$k]
    $prev connect $net
    [$buf findITerm A] connect $net
    set prev [$buf findITerm Z]
    lappend bufs $buf
    incr k
  }
  set net [odb::dbNet_create $block wire_${p}_$k]
  set load [ref::make_inst BUF_X1 load_$p $length $y]
  $prev connect $net
  [$load findITerm A] connect $net

  set out_net [odb::dbNet_create $block out_$p]
  [$load findITerm Z] connect $out_net
  ref::make_bterm $out_net out_$p OUTPUT $length $y

  set path($p,i) $i
  set path($p,y) $y
  set path($p,length) $length
  set path($p,drvr) $drvr
  set path($p,bufs) $bufs
  return $p
}

proc ref::arrival { pin_name } {
  set vertex [[get_pin $pin_name] vertices]
  return [[sta::vertex_worst_arrival_path $vertex max] arrival]
}

# Path delay in ps as buffer_varying_lengths.tcl reports it: from the input
# external delay to the load output.
proc ref::time_all { } {
  variable npaths
  variable input_delay
  variable delay
  estimate_parasitics -placement
  sta::find_timing
  set offset [sta::time_ui_sta $input_delay]
  for { set p 0 } { $p < $npaths } { incr p } {
    set delay($p) [expr { ([ref::arrival load_$p/Z] - $offset) * 1e12 }]
  }
}

proc ref::driver_of { p } {
  variable path
  return [[$path($p,drvr) getMaster] getName]
}

# Buffer chain of path p as a list of {cell x} pairs, driver side first.
proc ref::chain_of { p } {
  variable path
  set chain {}
  foreach buf $path($p,bufs) {
    lappend chain [list [[$buf getMaster] getName] [lindex [$buf getLocation] 0]]
  }
  return $chain
}

# Solution file line: length index, delay (ps), driver cell, chain.
proc ref::format_solution { p } {
  variable path
  variable delay
  return [list $path($p,i) [format %.4f $delay($p)] [ref::driver_of $p] [ref::chain_of $p]]
}
