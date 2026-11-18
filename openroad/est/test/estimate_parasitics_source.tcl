# estimate_parasitics without a flag estimates from what the design has,
# placement here, and leaves parasitics read from a SPEF alone.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg3.def

create_clock -period 10 clk
set_input_delay -clock clk 0 in1

# The SPEF a file holds, without its date.
proc spef_body { filename } {
  set stream [open $filename r]
  set body {}
  foreach line [split [read $stream] "\n"] {
    if { ![string match {\*DATE*} $line] } {
      lappend body $line
    }
  }
  close $stream
  return $body
}

check "before an estimate there are no parasitics" {
  est::parasitics_source
} none

check "the design is placed" {
  grt::design_is_placed
} 1

check_error "a placement without wire RC is an error" {
  estimate_parasitics
} EST-0014

source Nangate45/Nangate45.rc
set_wire_rc -layer metal2

set placement_spef [make_result_file estimate_parasitics_placement.spef]
estimate_parasitics -placement -spef_file $placement_spef
set auto_spef [make_result_file estimate_parasitics_auto.spef]
estimate_parasitics -spef_file $auto_spef
check "without global routes, estimate_parasitics estimates from placement" {
  expr { [spef_body $auto_spef] == [spef_body $placement_spef] }
} 1
check "the parasitics come from placement" {
  est::parasitics_source
} placement

read_spef $placement_spef
set after_read_spef [make_result_file estimate_parasitics_after_read_spef.spef]
estimate_parasitics -spef_file $after_read_spef
check "after read_spef, estimate_parasitics does nothing" {
  file exists $after_read_spef
} 0
check "the parasitics come from the SPEF" {
  est::parasitics_source
} spef

estimate_parasitics -placement
set after_placement [make_result_file estimate_parasitics_after_placement.spef]
estimate_parasitics -spef_file $after_placement
check "an explicit estimate after read_spef makes estimate_parasitics estimate" {
  file exists $after_placement
} 1

[[ord::get_db_block] findInst u1] setPlacementStatus NONE
check "with an unplaced instance, the design is not placed" {
  grt::design_is_placed
} 0
check_error "an unplaced design is an error" {
  estimate_parasitics
} EST-0009

exit_summary
