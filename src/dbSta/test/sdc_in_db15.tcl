# Two modes through the text fallback. A derate in one mode puts the
# whole record outside the native form, so the constraints ride as
# write_sdc text -- which writes one mode at a time, and must still come
# back mode by mode.
source "sdc_in_db_common.tcl"
load_design sdc_in_db.v

set_mode func
create_clock -name clk -period 10 [get_ports clk1]
create_clock -name clk2 -period 30 [get_ports clk2]
set_input_delay 1 -clock clk [get_ports in1]

set_mode test
create_clock -name clk -period 20 [get_ports clk1]
create_clock -name clk2 -period 30 [get_ports clk2]
create_clock -name clk3 -period 40 [get_ports clk3]
set_input_delay 1 -clock clk [get_ports in1]
set_input_delay 1 -clock clk2 [get_ports in2]
set_false_path -from [get_ports in2]
set_timing_derate -late 1.05

set odb [make_result_file sdc_in_db15.odb]
write_db -sdc $odb
check "stored form" { ord::sdc_in_db_kind } text

set ::env(SDC_IN_DB_ODB) $odb
set child [run_child sdc_in_db_modes.tcl]
check "form seen by the restoring process" { child_kind $child } text
check "modes restored" { child_value $child modes } {func test}
check "clocks in func" { child_value $child func_clocks } {clk clk2}
check "clocks in test" { child_value $child test_clocks } {clk clk2 clk3}
check "clk period in func" { child_value $child func_period } 10
check "clk period in test" { child_value $child test_period } 20
check "input delays in func" { child_value $child func_input_delays } 1
check "input delays in test" { child_value $child test_input_delays } 2
check "exception in func" { child_value $child func_false_paths } 0
check "exception in test" { child_value $child test_false_paths } 1
check "derate in func" { child_value $child func_derates } 0
check "derate in test" { child_value $child test_derates } 1
exit_summary
