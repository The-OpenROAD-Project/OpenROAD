# Report cell usage for modinsts

source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog hier1.v
link_design top

report_cell_usage -verbose

report_cell_usage -verbose b1
report_cell_usage -verbose b2
