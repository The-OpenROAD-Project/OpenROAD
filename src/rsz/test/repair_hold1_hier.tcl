# repair_pin_hold_violations
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog repair_hold1_hier.v
link_design top -hier
create_clock -period 2 clk
set_input_delay -clock clk 0 {in1 in2}
set_propagated_clock clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

report_slack r3/D

rsz::repair_hold_pin [get_pins r3/D] 0.0 0.0 0 1.0 100

report_slack r3/D
set verilog_file [make_result_file repair_hold1_hier_out.v]
write_verilog $verilog_file
diff_files $verilog_file repair_hold1_hier_out.vok
