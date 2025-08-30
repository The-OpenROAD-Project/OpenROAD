# Check bus ports of hard macro instance in flat flow
source "helpers.tcl"

proc print_info { objs } {
  set obj_names {}
  foreach obj $objs {
    lappend obj_names [get_name $obj]
  }
  puts "[lsort $obj_names]"
  puts "count: [llength $objs]"
  puts ""
}

read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x7.lib
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef
read_verilog buffer_ports11.v
link_design top

print_info [get_ports clk]
print_info [get_ports ce_in]
print_info [get_ports we_in]
print_info [get_ports addr_in*]
print_info [get_ports wd_in*]
print_info [get_ports w_mask_in*]
print_info [get_ports rd_out*]
print_info [get_pins sram/clk]
print_info [get_pins sram/addr_in*]

buffer_ports

set verilog_file [make_result_file buffer_ports11.v]
write_verilog $verilog_file
diff_files buffer_ports11.vok $verilog_file
