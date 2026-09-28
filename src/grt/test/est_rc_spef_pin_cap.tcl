# estimate_parasitics -global_routing -spef_file must write *CAP for pin nodes
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def est_rc2.def

set_routing_layers -signal metal2-metal10

global_route
set spef_file [make_result_file est_rc_spef_pin_cap.spef]
estimate_parasitics -global_routing -spef_file $spef_file

set stream [open $spef_file r]
set spef [read $stream]
close $stream
# *CAP line is "<idx> <node> <cap>"; *RES lines have 4 fields
puts "u2:ZN *CAP: [regexp -line {^\d+ u2:ZN \S+$} $spef]"
