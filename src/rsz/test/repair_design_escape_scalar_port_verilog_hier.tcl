# repair_design buffering must preserve escaped scalar bracket ports from
# Verilog when the design is linked through hierarchical flow.
source "helpers.tcl"

set test_name repair_design_escape_scalar_port_verilog_hier
set input_name repair_design_escape_scalar_port_verilog_hier
set top_scalar_port {foo\[7\]}
set mod_scalar_port {bar\[3\]}
set top_output_port {out\[0\]}
set mod_output_port {out\[1\]}
set load_count 35

proc place_inst { inst_name x y } {
  set inst [[ord::get_db_block] findInst $inst_name]
  $inst setLocation $x $y
  $inst setOrient R0
  $inst setPlacementStatus PLACED
}

proc place_bterm { bterm_name layer_name x y } {
  set block [ord::get_db_block]
  set tech [ord::get_db_tech]
  set bterm [$block findBTerm $bterm_name]
  set layer [$tech findLayer $layer_name]
  set bpin [odb::dbBPin_create $bterm]
  $bpin setPlacementStatus PLACED
  odb::dbBox_create $bpin $layer $x $y [expr { $x + 100 }] [expr { $y + 100 }]
}

read_liberty repair_fanout4.lib
read_lef Nangate45/Nangate45.lef
read_verilog "${input_name}.v"
link_design -hier top

set child_mod [[ord::get_db_block] findModule child]
set mod_bterm [$child_mod findModBTerm $mod_scalar_port]
if { $mod_bterm == "NULL" } {
  utl::error RSZ 999 \
    "hierarchical read_verilog did not create dbModBTerm \\bar[3]"
}
set mod_out_bterm [$child_mod findModBTerm $mod_output_port]
if { $mod_out_bterm == "NULL" } {
  utl::error RSZ 999 \
    "hierarchical read_verilog did not create dbModBTerm \\out[1]"
}

initialize_floorplan \
  -die_area {0 0 30 40} \
  -core_area {0 0 30 40} \
  -site FreePDK45_38x28_10R_NP_162NW_34O

# Place the escaped scalar top port and its fanout loads without using DEF.
place_bterm $top_scalar_port metal1 0 1000
place_bterm $top_output_port metal1 29000 1000
place_inst u_child/out_driver 25000 1000
for { set i 0 } { $i < $load_count } { incr i } {
  set x [expr { ($i % 5) * 5000 }]
  set y [expr { ($i / 5) * 5000 }]
  place_inst u_child/load$i $x $y
}

set_max_fanout 3 [current_design]
set_driving_cell -lib_cell BUF_X1 [all_inputs]
set_load 100 [get_ports $top_output_port]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

report_check_types -max_fanout

repair_design
report_check_types -max_fanout

set verilog_file [make_result_file "${test_name}.v"]
write_verilog $verilog_file
report_file $verilog_file

set stream [open $verilog_file r]
set verilog_text [read $stream]
close $stream

set has_buffering [regexp {BUF_X1 fanout[0-9]+} $verilog_text]
set has_escaped_top_decl [regexp {input \\foo\[7\] ;} $verilog_text]
set has_escaped_mod_decl [regexp {input \\bar\[3\] ;} $verilog_text]
set has_escaped_top_out_decl [regexp {output \\out\[0\] ;} $verilog_text]
set has_escaped_mod_out_decl [regexp {output \\out\[1\] ;} $verilog_text]
set has_escaped_mod_ref [regexp {\(\.A\(\\bar\[3\] \)} $verilog_text]
set has_escaped_mod_inst_ref [regexp {\.\\bar\[3\] \(\\foo\[7\] \)} $verilog_text]
set has_escaped_mod_out_inst_ref [regexp {\.\\out\[1\] \(net[0-9]+\)} $verilog_text]
set has_escaped_output_buffer 0
if { [regexp {\.\\out\[1\] \((net[0-9]+)\)} $verilog_text -> output_net] } {
  set output_buffer_pattern [format \
    {BUF_X1 wire[0-9]+ \(\.A\(%s\),\n    \.Z\(\\out\[0\] \)\)} \
    $output_net]
  set has_escaped_output_buffer [regexp $output_buffer_pattern $verilog_text]
}
set has_bad_bus_port_decl [regexp {input \[7:7\] foo;} $verilog_text]
set has_bad_mod_bus_port_decl [regexp {input \[3:3\] bar;} $verilog_text]
set has_bad_out_bus_port_decl [regexp {output \[0:0\] out;} $verilog_text]
set has_bad_mod_out_bus_port_decl [regexp {output \[1:1\] out;} $verilog_text]
set has_bad_bus_select_ref [regexp {\(\.A\(foo\[7\]\)} $verilog_text]
set has_bad_mod_bus_select_ref [regexp {\(\.A\(bar\[3\]\)} $verilog_text]
set has_bad_out_bus_select_ref [regexp {\(\.Z\(out\[1\]\)} $verilog_text]

if { !$has_buffering } {
  utl::error RSZ 999 "repair_design did not buffer the escaped scalar port"
}
if { !$has_escaped_top_decl } {
  utl::error RSZ 999 \
    "write_verilog did not preserve escaped scalar top port \\foo[7]"
}
if { !$has_escaped_top_out_decl } {
  utl::error RSZ 999 \
    "write_verilog did not preserve escaped scalar top output port \\out[0]"
}
if {
  !$has_escaped_mod_decl || !$has_escaped_mod_ref
  || !$has_escaped_mod_inst_ref
} {
  utl::error RSZ 999 \
    "write_verilog did not preserve escaped scalar module port \\bar[3]"
}
if {
  !$has_escaped_mod_out_decl || !$has_escaped_mod_out_inst_ref
  || !$has_escaped_output_buffer
} {
  utl::error RSZ 999 \
    "repair_design did not buffer escaped scalar module output port \\out[1]"
}
if {
  $has_bad_bus_port_decl || $has_bad_mod_bus_port_decl
  || $has_bad_out_bus_port_decl || $has_bad_mod_out_bus_port_decl
  || $has_bad_bus_select_ref || $has_bad_mod_bus_select_ref
  || $has_bad_out_bus_select_ref
} {
  utl::error RSZ 999 \
    "write_verilog emitted bus syntax for an escaped scalar bracket port"
}

puts "pass"
