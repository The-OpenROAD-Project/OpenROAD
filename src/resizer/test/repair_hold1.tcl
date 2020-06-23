# repair_pin_hold_violations
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_hold1.def

create_clock -period 2 clk
set_input_delay -clock clk 0 {in1 in2}
set_propagated_clock clk

set_wire_rc -layer metal1
estimate_parasitics -placement

report_checks -path_delay min -format full_clock -endpoint_count 2 \
  -unique_paths_to_endpoint -to r3/D

sta::resizer_preamble [get_libs *]
sta::repair_pin_hold_violations [get_pins r3/D] [get_lib_cell BUF_X2]
report_checks -path_delay min -format full_clock -endpoint_count 2 \
  -unique_paths_to_endpoint -to r3/D

set def_file [make_result_file repair_hold1.def]
write_def $def_file
diff_files repair_hold1.defok $def_file
