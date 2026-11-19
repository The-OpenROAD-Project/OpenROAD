# Reload the database written by top_port_order_read_db.tcl in a fresh process.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_db [make_result_file top_port_order_read_db.odb]

puts "after read_db ports: [lmap p [get_ports *] { get_name $p }]"
write_verilog -include_pwr_gnd [make_result_file top_port_order_read_db_after_read_db.v]
write_timing_model [make_result_file top_port_order_read_db_after_read_db.lib]
