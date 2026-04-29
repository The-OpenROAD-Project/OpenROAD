# Regression for dbModule::findModBTerm handling of modbterms whose stored
# name contains '/' characters (as produced when Fusion Compiler emits an
# escaped-identifier port name into the netlist).  Before the findModBTerm
# fix, the pin-direction resolution below would call findModBTerm with the
# full name, get unconditionally truncated to the trailing segment in the
# lookup, miss, and crash on a null modbterm in dbNetwork::direction.
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog hier_weird_port.v
link_design top -hier

# This was the call path that crashed on the ebrick_south flow.
check_setup -verbose

# Confirm the modbterm is stored with '/' chars preserved.
set block [ord::get_db_block]
set top_mod [$block getTopModule]
foreach child [$top_mod getChildren] {
  set master [$child getMaster]
  puts "modinst '[$child getName]' master '[$master getName]':"
  set mbt [$master getHeadDbModBTerm]
  if { $mbt != "NULL" } {
    puts "  head modbterm: '[$mbt getName]'"
  }
}

# Pin-direction resolution on u_sub.
puts "--- pin-direction on u_sub ---"
foreach pin [get_pins -hierarchical u_sub/*] {
  puts "  [get_property $pin full_name] dir=[get_property $pin direction]"
}
