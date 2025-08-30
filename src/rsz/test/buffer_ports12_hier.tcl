# Check bus ports of hardmacro instances in hierarchical flow
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
read_verilog buffer_ports12.v
link_design -hier top

print_info [get_ports clk]
print_info [get_ports sram0_ce_in]
print_info [get_ports sram0_we_in]
print_info [get_ports sram0_addr_in*]
print_info [get_ports sram0_w_mask_in*]
print_info [get_ports sram1_ce_in]
print_info [get_ports sram1_we_in]
print_info [get_ports sram1_addr_in*]
print_info [get_ports sram1_w_mask_in*]
print_info [get_pins sram0/clk]
print_info [get_pins sram0/addr_in*]
print_info [get_pins sram1/clk]
print_info [get_pins sram1/addr_in*]

buffer_ports

set verilog_file [make_result_file buffer_ports12_hier.v]
write_verilog $verilog_file
diff_files buffer_ports12_hier.vok $verilog_file
