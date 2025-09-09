# Report cell usage for modinsts with metrics

source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog hier1.v
link_design top

set metrics [make_result_file report_cell_usage_modinsts_metrics.json]

utl::open_metrics $metrics

report_cell_usage -verbose

report_cell_usage -verbose b1
report_cell_usage -verbose b2

utl::close_metrics $metrics

diff_files report_cell_usage_modinsts_metrics.jsonok $metrics
