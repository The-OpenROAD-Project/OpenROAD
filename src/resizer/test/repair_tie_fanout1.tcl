# repair high fanout tie hi/low net
source helpers.tcl
read_liberty Nangate_typ.lib
read_lef Nangate.lef

set verilog_filename [file join $result_dir "tie_hi_fanout1.v"]

proc write_tie_hi_fanout_netlist { filename tie_port load_port fanout } {
  set stream [open $filename "w"]
  puts $stream "module top ();"
  set tie_port1 [get_lib_pin $tie_port]
  set tie_cell_name [get_property [get_property $tie_port1 lib_cell] name]
  set tie_port_name [get_property $tie_port1 name]
  puts $stream " $tie_cell_name t1 (.${tie_port_name}(n1));"
  set load_port1 [get_lib_pin $load_port]
  set load_cell_name [get_property [get_property $load_port1 lib_cell] name]
  set load_port_name [get_property $load_port1 name]
  for {set i 0} {$i < $fanout} {incr i} {
    set inst_name "u$i"
    puts $stream " $load_cell_name $inst_name (.${load_port_name}(n1));"
  }
  puts $stream "endmodule"
  close $stream
}

write_tie_hi_fanout_netlist $verilog_filename \
  Nangate_typ/LOGIC1_X1/Z Nangate_typ/BUF_X1/A 92

read_verilog $verilog_filename
link_design top
repair_tie_fanout -max_fanout 10 -verbose Nangate_typ/LOGIC1_X1/Z
set tie_insts [get_cells -filter "ref_name == LOGIC1_X1"]
foreach tie_inst $tie_insts {
  set tie_pin [get_pins -of $tie_inst -filter "direction == output"]
  set net [get_nets -of $tie_inst]
  set fanout [llength [get_pins -of $net -filter "direction == input"]]
  puts "[get_full_name $tie_inst] $fanout"
}
