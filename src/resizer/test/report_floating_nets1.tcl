# report_floating_nets
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog gcd_yosys.v
link_design gcd
report_floating_nets
report_floating_nets -verbose
