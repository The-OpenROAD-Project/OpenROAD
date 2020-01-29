source helpers.tcl

set verilog_filename [file join $result_dir "repair_max_fanout1.v"]

proc write_hi_fanout_netlist { filename fanout } {
  set stream [open $filename "w"]
  puts $stream "module top (clk1, in1);"
  puts $stream " input clk1, in1;"
  puts $stream " snl_bufx1 u1 (.A(in1), .Z(u1z));"
  for {set i 0} {$i < $fanout} {incr i} {
    set reg_name "r$i"
    # constant value for sim updates
    puts $stream " snl_ffqx1 $reg_name (.CP(clk1), .D(u1z));"
  }
  puts $stream "endmodule"
  close $stream
}

write_hi_fanout_netlist $verilog_filename 35

read_liberty liberty1.lib
read_lef liberty1.lef
read_verilog $verilog_filename
link_design top
create_clock -period 10 clk1
repair_max_fanout -max_fanout 10 -buffer_cell liberty1/snl_bufx1
report_object_names [get_pins -of [get_net u1z]]
report_object_names [get_pins -of [get_net net1]]
report_object_names [get_pins -of [get_net net2]]
report_object_names [get_pins -of [get_net net3]]
report_object_names [get_pins -of [get_net net4]]
