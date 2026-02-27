#
# replace_hier_mod2
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

estimate_parasitics -placement

report_checks -from {dpath.b_reg.out[4]$_DFFE_PP_/CLK} -through _carry_out_and_/C -field input
report_cell_usage _551_

# For eqy, write a verilog before replace_hier_module
write_verilog_for_eqy replace_hier_mod2 before "None"

#set_debug_level ODB replace_design 1
set_debug_level ODB replace_design_check_sanity 1

# Swap #1. BRENT_KUNG -> KOGGE_STONE
replace_hier_module _551_ LCU_16_KOGGE_STONE
estimate_parasitics -placement
report_checks -from {dpath.b_reg.out[4]$_DFFE_PP_/CLK} -through _carry_out_and_/C -field input
report_cell_usage _551_
# Skip EQY because BRENT_KUNG and KOGGE_STONE are not equivalent.

# Swap #2. KOGGE_STONE -> BRENT_KUNG (Rollback)
replace_hier_module _551_ LCU_16_BRENT_KUNG
estimate_parasitics -placement
report_checks -from {dpath.b_reg.out[4]$_DFFE_PP_/CLK} -through _carry_out_and_/C -field input
report_cell_usage _551_
puts "Checking equivalence after swap #2 (Rollback)..."
run_equivalence_test replace_hier_mod2 \
  -lib_dir ./sky130hd/work_around_yosys/ \
  -remove_cells "None"

# Swap #3. BRENT_KUNG -> KOGGE_STONE (Redo)
replace_hier_module _551_ LCU_16_KOGGE_STONE
estimate_parasitics -placement
report_checks -from {dpath.b_reg.out[4]$_DFFE_PP_/CLK} -through _carry_out_and_/C -field input
report_cell_usage _551_

# Swap #4. KOGGE_STONE -> BRENT_KUNG (Rollback again)
replace_hier_module _551_ LCU_16_BRENT_KUNG
estimate_parasitics -placement
report_checks -from {dpath.b_reg.out[4]$_DFFE_PP_/CLK} -through _carry_out_and_/C -field input
report_cell_usage _551_
puts "Checking equivalence after swap #4 (Rollback again)..."
run_equivalence_test replace_hier_mod2 \
  -lib_dir ./sky130hd/work_around_yosys/ \
  -remove_cells "None"
