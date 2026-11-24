# Exercise bottom-up buffering with macro insertion delays and register trees.
source "helpers.tcl"
source "bottom_up_audit_helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef array_tile_ins_delay.lef
read_liberty Nangate45/Nangate45_typ.lib
read_liberty array_tile_ins_delay.lib
read_def insertion_delay.def
create_clock -name core -period 5 clk
set_input_transition 0.02 [get_ports clk]
set_wire_rc -clock -layer metal3
set_wire_rc -signal -layer metal3
configure_cts_characterization -slew_steps 3 -cap_steps 5

set block [ord::get_db_block]
set original_sinks {}
foreach iterm [[$block findNet clk] getITerms] {
  if { [$iterm isInputSignal] } {
    lappend original_sinks $iterm
  }
}
tee -quiet -variable log {
  clock_tree_synthesis -bottom_up_buffering -sink_clustering_enable \
    -root_buf BUF_X8 -buf_list {BUF_X1 BUF_X2 BUF_X4 BUF_X8} \
    -distance_between_buffers 10
}
check "macro and register trees are separated" {
  expr { [llength [get_cells -quiet clkbuf_regs_*]] > 0 }
} 1
check "macro/register search reports bounded alternatives" {
  string match {*frontier <= *} $log
} 1
check "audit covers the completed balanced clock tree" {
  string match {*0 max-cap and 0 max-slew violations*} $log
} 1
set connected 0
foreach pin $original_sinks {
  if { [$pin getNet] != "NULL" } {
    incr connected
  }
}
check "macro and register sinks remain connected" {
  set connected
} [llength $original_sinks]
check_bottom_up_paths $block $original_sinks $log
exit_summary
