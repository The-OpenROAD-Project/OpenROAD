# Test that read_def handles port names with non-numeric bus subscripts
# like kv_write[3][write_en] without crashing (stoi in parseBusName).
# Also verify that ports with numeric trailing subscripts are grouped
# into buses correctly.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def read_def_bus_port.def

# Verify read_def completed (no crash on non-numeric subscripts)
set all_ports [get_ports *]
puts "port_count: [llength $all_ports]"

# Scalar port with non-numeric subscript should exist
puts "scalar: [get_name [get_ports {kv_write[3][write_en]}]]"

# Bus ports: kv_write[3][write_entry] should be grouped (bits 0-4)
set entry_ports [get_ports {kv_write[3][write_entry][*]}]
puts "write_entry_count: [llength $entry_ports]"

# Bus ports: kv_write[3][write_offset] should be grouped (bits 2-3)
set offset_ports [get_ports {kv_write[3][write_offset][*]}]
puts "write_offset_count: [llength $offset_ports]"

puts "clk: [get_name [get_ports clk]]"
