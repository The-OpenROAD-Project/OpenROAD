# PARALLEL_EVAL size-ups of scan flops whose fanin drivers have no max-cap
# headroom.  As in LEGACY, the scan-in and scan-enable pins skip the fanin
# max-cap check.  STA does not check the max cap of an ideal clock net, and a
# propagated clock net keeps the check.
source "helpers.tcl"

read_liberty sky130hd/sky130hd_tt.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_verilog repair_setup_parallel_eval_fanin_max_cap.v
link_design top

create_clock -name clk -period 2 [get_ports clk]
create_clock -name clk_prop -period 2 [get_ports clk_prop]
set_propagated_clock [get_clocks clk_prop]
# The late propagated clock would fail hold on the clk inputs of ff_prop and
# block its size-up for a reason other than max cap.
set_false_path -hold -from [get_clocks clk] -to [get_clocks clk_prop]
set_input_delay 0 -clock clk [get_ports {d se si}]
set_output_delay 1.5 -clock clk [get_ports {q_scan q_ideal}]
set_output_delay 1.5 -clock clk_prop [get_ports q_prop]
set_load 0.1 [get_ports {q_scan q_ideal q_prop}]
# Push the drivers of the tested pins past their max cap.
set_load 0.14 [get_ports {se_full si_full clk_full clk_prop_full}]

source sky130hd/sky130hd.rc
set_wire_rc -layer met2
estimate_parasitics -placement

# STA leaves the ideal clock net of drv_clk_full unchecked.
report_check_types -max_capacitance -violators

# sdfxbp_2 has larger CLK, SCE and SCD pins than sdfxbp_1.
repair_timing -setup -phases PARALLEL_EVAL -sequence sizeup

# ff_scan and ff_ideal size up; ff_prop keeps sdfxbp_1.
foreach inst {ff_scan ff_ideal ff_prop} {
  puts "$inst [get_property [get_cells $inst] ref_name]"
}
