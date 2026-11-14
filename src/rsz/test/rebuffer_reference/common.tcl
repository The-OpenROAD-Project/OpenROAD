# Shared setup for the buffer_varying_lengths reference search.
#
# Each candidate is an independent path built like one path of
# buffer_varying_lengths.tcl: input port -> driver -> buffer chain -> load
# buffer -> output port, with the load placed i length steps from the driver.
# Paths are stacked vertically so many candidates are timed in one STA run.
#
# Environment: REF_PDK (a buffer_varying_lengths_pdk.tcl PDK).
# Run from src/rsz/test so the PDK paths resolve.

set buffer_varying_lengths_pdk $::env(REF_PDK)
source "buffer_varying_lengths_pdk.tcl"

namespace eval ref {
variable cells $::bvl_buffers
variable npaths 0
}

proc ref::init { } {
  variable block
  variable pin_layer
  variable masters
  variable cells
  variable step
  bvl_read_pdk
  set db [ord::get_db]
  set tech [$db getTech]
  set chip [odb::dbChip_create $db $tech]
  set block [odb::dbBlock_create $chip top]
  $block setDefUnits [$tech getDbUnitsPerMicron]
  set pin_layer [$tech findLayer $::bvl_pin_layer]
  set step [expr { round($::bvl_step_um * [$tech getDbUnitsPerMicron]) }]
  foreach cell $cells {
    set masters($cell) [$db findMaster $cell]
  }
}

# Same constraints as the tight-timing step of buffer_varying_lengths.tcl.
proc ref::constrain { } {
  create_clock -period $::bvl_period -name clk
  set_input_delay -clock clk $::bvl_period [all_inputs]
  set_output_delay -clock clk 0.0 [all_outputs]
  set_driving_cell -lib_cell $::bvl_buffer -pin $::bvl_buffer_out [all_inputs]
  bvl_set_wire_rc
}

proc ref::length_dbu { i } {
  variable step
  return [expr { $i * $step }]
}

# A distance given in length steps, in DBU.
proc ref::steps_dbu { steps } {
  variable step
  return [expr { round($steps * $step) }]
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
  variable pin_layer
  set bterm [odb::dbBTerm_create $net $name]
  $bterm setIoType $io_type
  set bpin [odb::dbBPin_create $bterm]
  odb::dbBox_create $bpin $pin_layer $x $y [expr { $x + 100 }] [expr { $y + 100 }]
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
  set in_pin $::bvl_buffer_in
  set out_pin $::bvl_buffer_out
  set p $npaths
  incr npaths
  set length [ref::length_dbu $i]
  set y [expr { $p * 4000 }]

  set in_net [odb::dbNet_create $block in_$p]
  ref::make_bterm $in_net in_$p INPUT 0 $y
  set drvr [ref::make_inst $driver drvr_$p 0 $y]
  [$drvr findITerm $in_pin] connect $in_net
  set prev [$drvr findITerm $out_pin]

  set bufs {}
  set k 0
  foreach cell $seq x $xs {
    set buf [ref::make_inst $cell buf_${p}_$k $x $y]
    set net [odb::dbNet_create $block wire_${p}_$k]
    $prev connect $net
    [$buf findITerm $in_pin] connect $net
    set prev [$buf findITerm $out_pin]
    lappend bufs $buf
    incr k
  }
  set net [odb::dbNet_create $block wire_${p}_$k]
  set load [ref::make_inst $::bvl_buffer load_$p $length $y]
  $prev connect $net
  [$load findITerm $in_pin] connect $net

  set out_net [odb::dbNet_create $block out_$p]
  [$load findITerm $out_pin] connect $out_net
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
  variable delay
  estimate_parasitics -placement
  sta::find_timing
  set offset [sta::time_ui_sta $::bvl_period]
  for { set p 0 } { $p < $npaths } { incr p } {
    set arrival [ref::arrival load_$p/$::bvl_buffer_out]
    set delay($p) [expr { ($arrival - $offset) * 1e12 }]
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
