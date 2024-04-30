# hieararchical verilog
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog hier1.v
set_debug_level ODB  "dbReadVerilog" 1
link_design top -hier 
