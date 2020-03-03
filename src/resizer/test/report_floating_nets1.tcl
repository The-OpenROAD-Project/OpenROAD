# report_floating_nets
read_liberty Nangate_typ.lib
read_lef Nangate.lef
read_verilog gcd_yosys.v
link_design gcd
report_floating_nets
report_floating_nets -verbose
