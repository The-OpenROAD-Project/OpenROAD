# Run the tight-timing step of buffer_varying_lengths.tcl and write rsz's
# solution for every length in the solution file format of common.tcl.
#
# Environment: REF_SIZE_DRIVER (0 or 1), REF_OUT (output file).
# Run from src/rsz/test.

source "helpers.tcl"
set buffer_varying_lengths_size_driver $::env(REF_SIZE_DRIVER)
set buffer_varying_lengths_tight_only 1
source "buffer_varying_lengths.tcl"

proc chain_after { iterm load_name } {
  set chain {}
  while { 1 } {
    set next ""
    foreach load [[$iterm getNet] getITerms] {
      if { [$load isInputSignal] } {
        set next $load
      }
    }
    set inst [$next getInst]
    if { [$inst getName] == $load_name } {
      return $chain
    }
    lappend chain [list [[$inst getMaster] getName] [lindex [$inst getLocation] 0]]
    set iterm [$inst findITerm Z]
  }
}

set block [ord::get_db_block]
set offset [sta::time_ui_sta 1.0]
set out [open $::env(REF_OUT) w]
for { set i 1 } { $i <= 100 } { incr i } {
  set drvr [$block findInst i1-$i]
  set arrival [[sta::vertex_worst_arrival_path [[get_pin i2-$i/Z] vertices] max] arrival]
  set delay [expr { ($arrival - $offset) * 1e12 }]
  set chain [chain_after [$drvr findITerm Z] i2-$i]
  puts $out [list $i [format %.4f $delay] [[$drvr getMaster] getName] $chain]
}
close $out
