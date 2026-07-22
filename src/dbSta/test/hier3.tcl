# hierarchical verilog sample with unconnected pins
source "helpers.tcl"

set test_name "hier3"

read_lef example1.lef
read_liberty example1_typ.lib
read_verilog hier3.v
link_design top -hier

puts "Find b1/out2: [get_property [get_pins b1/out2] full_name]"
puts "Find b2/out2: [get_property [get_pins b2/out2] full_name]"
puts "Find bus pin b3/bus_out: [get_property [get_pins b3/bus_out[0]] full_name]"

set bus_pins [get_pins b3/bus_out]
foreach pin $bus_pins {
  puts "bus_out pin: [get_property $pin full_name]"
}

# Check if net and modnet are connected to "b2/u3/Z"
set block [ord::get_db_block]
set inst [$block findInst "b2/u3"]
set iterm [$inst findITerm "Z"]
set net [$iterm getNet]
set modnet [$iterm getModNet]
if { $net != "NULL" } {
  puts "Net connected to b2/u3/Z: [$net getName]"
} else {
  puts "b2/u3/Z is not connected to any net."
}

if { $modnet != "NULL" } {
  puts "Modnet connected to b2/u3/Z: [$modnet getName]"
} else {
  puts "b2/u3/Z is not connected to any modnet."
}

set v_file [make_result_file ${test_name}_out.v]
write_verilog $v_file
diff_files $v_file $test_name.vok
