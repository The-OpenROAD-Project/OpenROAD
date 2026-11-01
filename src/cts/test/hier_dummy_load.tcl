# Every load CTS adds to a hierarchical netlist, the dummy loads that
# balance a level among them, is on its net in the hierarchical view.
# A pin on the flat net alone is not seen once the database is read back.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog simple_test_hier.v
link_design test_16_sinks -hier
read_def -floorplan_initialize simple_test_hier.def

create_clock -period 5 clk
set_wire_rc -clock -layer metal3
set_cts_config -wire_unit 20 -apply_ndr root_only \
  -root_buf CLKBUF_X3 -buf_list CLKBUF_X3

clock_tree_synthesis -sink_clustering_enable

# Pins of the clock nets that have a hierarchical net but are not on one.
proc pins_off_hier_net { } {
  set off {}
  foreach net [[ord::get_db_block] getNets] {
    if { [$net getSigType] != "CLOCK" } {
      continue
    }
    set on_hier_net 0
    set flat_only {}
    foreach iterm [$net getITerms] {
      if { [$iterm getModNet] == "NULL" } {
        lappend flat_only [$iterm getName]
      } else {
        set on_hier_net 1
      }
    }
    if { $on_hier_net } {
      lappend off {*}$flat_only
    }
  }
  return $off
}

check "CTS adds a dummy load" \
  { expr { [llength [get_cells -quiet clkload*]] > 0 } } 1
check "no clock pin is off its hierarchical net" { pins_off_hier_net } {}
exit_summary
