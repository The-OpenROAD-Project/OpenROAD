#
# replace_design2
#
# This gcd instantiates two LCU units:
# 1) LCU_16_BRENT_KUNG instance _551_ (62 cells)
# 2) LCU_16_KOGGE_STONE instance _552_ (103 cells)
#
# Instance _551_ is swapped from BK to KS LCU.
#
source "helpers.tcl"

read_lef "data/sky130hd/sky130_fd_sc_hd.tlef"
read_lef "data/sky130hd/sky130_fd_sc_hd_merged.lef"

read_liberty "sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib"
read_verilog gcd_adder.v
link_design -hier gcd
create_clock -name CLK -period 1 clk
set_max_delay -to carry_out 1.0

report_checks -through _carry_out_and_/C -field input
report_cell_usage _551_

#set_debug_level ODB replace_design 1
replace_design _551_ LCU_16_KOGGE_STONE

report_checks -through _carry_out_and_/C -field input
report_cell_usage _551_

run_equivalence_test replace_design2 ./Nangate45/work_around_yosys/ "None"
