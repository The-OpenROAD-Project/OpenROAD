# buffer chain with set_max_delay
# repair_timing should not degrade existing max_slew or max_cap violations
source "helpers.tcl"
if { ![info exists repair_args] } {
  set repair_args {}
}
read_liberty sky130hd/sky130hd_tt.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def repair_setup5.def

source sky130hd/sky130hd.rc
set_wire_rc -layer met2
estimate_parasitics -placement

set_max_delay -from u1/X -to u5/A 2
report_worst_slack -max

# Get information so we can setup the test outputs correctly
write_verilog_for_eqy repair_setup5 before "None"
report_check_types -max_cap -max_slew
repair_timing -setup -repair_tns 100 {*}$repair_args
report_check_types -max_cap -max_slew
run_equivalence_test repair_setup5 \
  -lib_dir ./sky130hd/work_around_yosys/ \
  -remove_cells "None"
report_worst_slack -max
