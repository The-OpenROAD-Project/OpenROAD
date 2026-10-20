# get_nets glob and exact matching in hierarchical flow
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

read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog hier1.v
link_design top -hier

# top module nets
print_info [get_nets *]
print_info [get_nets b1out]
print_info [get_nets clk*]

# nets inside hierarchical instances: a glob must find what the
# exact lookup finds
print_info [get_nets b1/*]
print_info [get_nets b1/u1out]
print_info [get_nets b1/u1*]
print_info [get_nets b1/r1q]
print_info [get_nets b2/*]
print_info [get_nets */r1q]

# hierarchical search from the top
print_info [get_nets -hierarchical u1out]
print_info [get_nets -hierarchical u1*]

# pin glob over the same scope
print_info [get_pins b1/*]
