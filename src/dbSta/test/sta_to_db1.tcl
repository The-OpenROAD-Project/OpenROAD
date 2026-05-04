# Exercise the sta::sta_to_db_* SWIG bindings on a flat design.
#
# All instances/pins/nets/ports are leaves (no dbModInst / dbModNet /
# dbModITerm / dbModBTerm exist), so:
#   * sta_to_db_inst, _pin, _port, _net resolve to the matching dbInst /
#     dbITerm / dbBTerm / dbNet.
#   * sta_to_db_mod_inst, _mod_pin, _mod_port, _mod_net all return NULL.
source "helpers.tcl"

read_lef example1.lef
read_liberty example1_slow.lib
read_def example1.def

proc check { label obj } {
  if { $obj == "NULL" } {
    puts "$label: NULL"
    return
  }
  set name [$obj getName]
  if { [catch { set hier [$obj getHierarchicalName] }] } {
    puts "$label: $name"
  } else {
    puts "$label: $name (hier: $hier)"
  }
}

proc check_port { label port } {
  if { $port == "NULL" } {
    puts "$label: <no port>"
  } else {
    puts "$label: [get_property $port full_name]"
  }
}

# --- leaf instance ---
puts "# leaf instance r1"
set inst [get_cells r1]
check "  sta_to_db_inst" [sta::sta_to_db_inst $inst]
check "  sta_to_db_mod_inst" [sta::sta_to_db_mod_inst $inst]

# --- leaf instance pin ---
puts "# leaf pin r1/D"
set pin [get_pin r1/D]
check "  sta_to_db_pin" [sta::sta_to_db_pin $pin]
check "  sta_to_db_mod_pin" [sta::sta_to_db_mod_pin $pin]
check_port "  sta_pin_to_port" [sta::sta_pin_to_port $pin]

# --- top-level Port ---
puts "# top-level port clk1"
set port [lindex [get_ports clk1] 0]
check "  sta_to_db_port" [sta::sta_to_db_port $port]
check "  sta_to_db_mod_port" [sta::sta_to_db_mod_port $port]

# --- a flat dbNet ---
puts "# net r1q"
set net [get_net r1q]
check "  sta_to_db_net" [sta::sta_to_db_net $net]
check "  sta_to_db_mod_net" [sta::sta_to_db_mod_net $net]

# --- LibertyCell -> dbMaster, LibertyPort -> dbMTerm ---
puts "# liberty cell BUF_X1"
set lib_cell [get_property [get_cells u1] liberty_cell]
check "  sta_to_db_master" [sta::sta_to_db_master $lib_cell]

puts "# liberty port BUF_X1/A"
set lib_port [lindex [get_lib_pins NangateOpenCellLibrary_slow/BUF_X1/A] 0]
check "  sta_to_db_mterm" [sta::sta_to_db_mterm $lib_port]
