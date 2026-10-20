# Bottom-up CTS must insert real repeater stages on short-distance branches.
source "helpers.tcl"
source "bottom_up_audit_helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def 16sinks.def
create_clock -period 5 clk
set_input_transition 0.02 [get_ports clk]
set_wire_rc -clock -layer metal3
set_wire_rc -signal -layer metal3
configure_cts_characterization -slew_steps 3 -cap_steps 5
set block [ord::get_db_block]
set originals [[$block findNet clk] getITerms]

tee -quiet -variable log {
  clock_tree_synthesis -bottom_up_buffering -no_obstruction_aware \
    -root_buf BUF_X8 -buf_list {BUF_X1 BUF_X2 BUF_X4 BUF_X8} \
    -distance_between_buffers 1 -no_insertion_delay
}

check "bottom-up strategy runs" { string match {*Bottom-up buffered*} $log } 1
check "electrical audit passes" {
  string match {*0 max-cap and 0 max-slew violations*} $log
} 1
check "extra electrical stages are present" {
  expr { [llength [get_cells -quiet clkbuf_bottom_up_*]] > 0 }
} 1
check "new bottom-up nets are connected" {
  expr { [llength [get_nets -quiet clknet_bottom_up_*]] > 0 }
} 1
set connected 0
foreach pin [get_pins ff*/CK] {
  if { [llength [get_nets -of_objects $pin]] == 1 } {
    incr connected
  }
}
check "all original sinks remain connected" { set connected } 16
check "different loads permit small buffers" {
  expr { [llength [get_cells -quiet -filter {ref_name == BUF_X1} clkbuf_*]] > 0 }
} 1
check_bottom_up_paths $block $originals $log
exit_summary
