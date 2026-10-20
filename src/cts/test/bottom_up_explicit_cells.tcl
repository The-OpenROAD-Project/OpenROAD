# Explicit root and tree cells must survive sizing and electrical insertion.
source "helpers.tcl"
source "bottom_up_audit_helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def 16sinks.def
create_clock -period 5 clk
set_wire_rc -clock -layer metal3
set_wire_rc -signal -layer metal2
configure_cts_characterization -slew_steps 2 -cap_steps 2
set block [ord::get_db_block]
set originals [[$block findNet clk] getITerms]
tee -quiet -variable log {
  clock_tree_synthesis -bottom_up_buffering -no_obstruction_aware \
    -root_buf BUF_X8 -tree_buf BUF_X2 -buf_list {BUF_X1 BUF_X2 BUF_X8} \
    -distance_between_buffers 1 -no_insertion_delay
}
set extra 0
foreach inst [$block getInsts] {
  set name [$inst getName]
  if { $name == "clkbuf_0_clk" } {
    check "explicit root master" { [$inst getMaster] getName } BUF_X8
  } elseif { [string match "clkbuf_*" $name] } {
    check "explicit tree master on $name" { [$inst getMaster] getName } BUF_X2
    if { [string match "clkbuf_bottom_up_*" $name] } {
      incr extra
    }
  }
}
check "explicit cells used in added stages" { expr { $extra > 0 } } 1
check_bottom_up_paths $block $originals $log
exit_summary
