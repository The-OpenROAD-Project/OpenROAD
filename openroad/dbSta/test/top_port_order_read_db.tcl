# Top ports created after link_design (pdngen -pins, make_port) must be
# ordered the same in write_verilog and write_timing_model output as after
# read_db rebuilds the top cell: scalar ports first, bus ports last.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog top_port_order_read_db.v
link_design top

# Supply bterms created after linking, as pdngen -pins does.
set block [ord::get_db_block]
foreach {name sig_type} {VDD POWER VSS GROUND} {
  set net [odb::dbNet_create $block $name]
  $net setSigType $sig_type
  set bterm [odb::dbBTerm_create $net $name]
  $bterm setSigType $sig_type
  $bterm setIoType INOUT
}
make_port late_in input

puts "in-session ports: [lmap p [get_ports *] { get_name $p }]"
set in_session_v [make_result_file top_port_order_read_db_in_session.v]
write_verilog -include_pwr_gnd $in_session_v
set in_session_lib [make_result_file top_port_order_read_db_in_session.lib]
write_timing_model $in_session_lib

write_db [make_result_file top_port_order_read_db.odb]

# Reload in a separate process so the top cell is rebuilt from the bterms.
puts [exec [info nameofexecutable] -no_splash -no_init -exit \
  [file join [file dirname [info script]] "top_port_order_read_db_reload.tcl"] 2>@1]

diff_files $in_session_v \
  [make_result_file top_port_order_read_db_after_read_db.v]
diff_files $in_session_lib \
  [make_result_file top_port_order_read_db_after_read_db.lib]
