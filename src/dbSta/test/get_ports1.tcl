# get_ports on a bus
source "helpers.tcl"

# tclint-disable
proc print_info { objs } {
  set obj_names {}
  foreach obj $objs {
    lappend obj_names [get_name $obj]
  }
  puts "[lsort $obj_names]"
  puts "count: [llength $objs]"
  puts ""
}
# tclint-enable

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog get_ports1.v
link_design top

print_info [get_ports *]

# top module ports
print_info [get_ports "top_in_bus*"]
print_info [get_ports "top_in_bus*"]
print_info [get_ports "top_in_single*"]
print_info [get_ports "top_out_single*"]
print_info [get_ports top_in*]
print_info [get_ports top_out*]
print_info [get_ports clk]

# sub_module port - WARNINGS
print_info [get_cells "sub_inst"]
print_info [get_pins "sub_inst/in_bus*"]
print_info [get_pins "sub_inst/out_bus*"]
print_info [get_pins "sub_inst/in_single*"]
print_info [get_pins "sub_inst/out_single*"]
print_info [get_pins sub_inst/in*]
print_info [get_pins sub_inst/out*]
print_info [get_pins sub_inst/*[1]]

# Test getting a bit of a bus
print_info [get_ports top_in_bus[3]]
print_info [get_ports top_in_bus[2]]
print_info [get_ports top_in_bus[1]]
print_info [get_ports top_in_bus[0]]
print_info [get_ports *[2]]
