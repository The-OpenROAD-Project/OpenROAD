# report_fmax_metric leaves virtual clocks out of timing__fmax
source "helpers.tcl"
set test_name report_fmax_metric_virtual_clock

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog ${test_name}.v
link_design top

# Real clock for the registers, virtual clock for the I/O delays
create_clock -name core_clock -period 1.0 [get_ports clk]
create_clock -name vclk_core_clock -period 1.0
set_input_delay 0.05 -clock vclk_core_clock [all_inputs -no_clocks]
set_output_delay 0.05 -clock vclk_core_clock [all_outputs]

set metrics [make_result_file ${test_name}.json]
utl::open_metrics $metrics

# The virtual clock keeps its own metric but does not set timing__fmax
report_fmax_metric

# With only the virtual clock left, timing__fmax falls back to it
delete_clock [get_clocks core_clock]
utl::push_metrics_stage "vclk_only__{}"
report_fmax_metric
utl::pop_metrics_stage

utl::close_metrics $metrics
report_file $metrics
puts ""
