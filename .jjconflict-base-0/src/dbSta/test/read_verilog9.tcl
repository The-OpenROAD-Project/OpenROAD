# read_verilog/link/repeat twit proofing
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog reg1.v
link_design top

read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog reg1.v
link_design top
