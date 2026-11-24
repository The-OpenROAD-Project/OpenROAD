# clock_tree_synthesis -delay_balanced reaches the tree builder's option; the
# tree itself is not built.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "16sinks.def"

create_clock -period 5 clk
set_wire_rc -clock -layer metal3

rename cts::run_triton_cts cts::saved_run_triton_cts
proc cts::run_triton_cts { } { }
rename cts::set_delay_balanced cts::saved_set_delay_balanced
proc cts::set_delay_balanced { enable } {
  set ::delay_balanced $enable
  cts::saved_set_delay_balanced $enable
}

clock_tree_synthesis -root_buf CLKBUF_X3 -buf_list CLKBUF_X3 -delay_balanced
check "-delay_balanced turns the option on" { expr { !!$::delay_balanced } } 1

clock_tree_synthesis -root_buf CLKBUF_X3 -buf_list CLKBUF_X3
check "the option is off without the flag" { expr { !!$::delay_balanced } } 0

exit_summary
