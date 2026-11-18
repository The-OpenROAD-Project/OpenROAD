#
# replace_hier_mod4
#
# Test if uninstantiated module can be swapped
#
# top instantiates gcd module two times: gcd_1, gcd_2
# Each gcd instantiates two LCU_16_KOGGE_STONE modules
# LCU_16_BRENT_KUNG is not instantiated
#
# top
#   gcd_1
#     _551_
#     _552_
#   gcd_2
#     _551_
#     _552_
#
# Instance gcd_1/_552_ is swapped from KS to BK LCU.
# Input design has only KOGGE_STONE LCUs.
# 1) LCU_16_BRENT_KUNG instance _551_ (62 cells)
# 2) LCU_16_KOGGE_STONE instance _552_ (103 cells)
#
# Instance _551_ is swapped from KS to BK LCU.
#
source "helpers.tcl"

read_lef "data/sky130hd/sky130_fd_sc_hd.tlef"
read_lef "data/sky130hd/sky130_fd_sc_hd_merged.lef"

read_liberty "sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib"
#set_debug_level ODB dbReadVerilog 1
#set_debug_level ODB replace_design 1
read_verilog gcd_adder4.v
link_design -hier top
create_clock -name CLK -period 1 clk
set_max_delay -to carry_out 1.0

report_checks -through gcd_1/_carry_out_and_/B -fields input_pins
report_cell_usage gcd_1/_552_

set_debug_level ODB replace_design_check_sanity 1
replace_hier_module gcd_1/_552_ LCU_16_BRENT_KUNG

report_checks -through gcd_1/_carry_out_and_/B -fields input_pins
report_cell_usage gcd_1/_552_
# Skip EQY because BRENT_KUNG and KOGGE_STONE are not equivalent.
