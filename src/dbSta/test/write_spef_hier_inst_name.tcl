# Cells whose in-module name has a divider of its own (issue #12049).
# No OpenROAD reader produces such a name; rename sets the state reported
# from XSTile.
source "helpers.tcl"

set test_name write_spef_hier_inst_name

source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog escape_slash_hier.v
link_design -hier multi_sink
read_def -floorplan_initialize escape_slash_hier.def

source Nangate45/Nangate45.rc
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk

set block [ord::get_db_block]
[$block findInst {h1/ff0\/name}] rename {h1/core\/dispatch/ff0}
[$block findInst h1/gclk_inst] rename {h1/core\/dispatch/gclk_inst}

foreach inst [get_cells -hier *] {
  puts "cell [get_full_name $inst] name [get_name $inst]"
}
foreach net [get_nets -hier *] {
  puts "net [get_full_name $net] name [get_name $net]"
}
foreach pin [get_pins -of_objects [get_nets h1/gclk]] {
  puts "h1/gclk pin [get_full_name $pin]"
}

set spef_file [make_result_file $test_name.spef]
estimate_parasitics -placement -spef_file $spef_file
diff_files $test_name.spefok $spef_file
