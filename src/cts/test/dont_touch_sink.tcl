# A dont_touch register on a clock net. Clock tree synthesis would have to
# reconnect its clock pin, which odb refuses for a dont_touch instance, so
# CTS says so before it changes the net (CTS-0137) instead of failing in
# odb (ODB-0370) with the tree half built.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "16sinks.def"

create_clock -period 5 clk
set_wire_rc -clock -layer metal3
set_cts_config -wire_unit 20 \
  -apply_ndr root_only \
  -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3

set block [ord::get_db_block]
set clk_net [$block findNet clk]

proc clock_sinks { net } {
  set names {}
  foreach iterm [$net getITerms] {
    if { [$iterm isInputSignal] } {
      lappend names [$iterm getName]
    }
  }
  return [lsort $names]
}

set flop ff3
set_dont_touch [get_cells $flop]

set sinks_before [clock_sinks $clk_net]
set insts_before [llength [$block getInsts]]

set failed [catch { tee -quiet -variable log { clock_tree_synthesis } } message]
check "a dont_touch sink stops CTS" { set failed } 1
check "the error is CTS-0137" { string match {*CTS-0137*} $message } 1
check "the error names the dont_touch sink" \
  { string match "*CTS-0137*$flop*" $log } 1
check "odb is not the one refusing" { string match {*ODB-0370*} $log } 0
check "the clock net keeps its sinks" { clock_sinks $clk_net } $sinks_before
check "no buffer was inserted" { llength [$block getInsts] } $insts_before

exit_summary
