#
# replace_hier_mod5
#
# Test undo feature for module swap
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

puts "### Initial module is Kogge-Stone ###"
report_cell_usage gcd_1/_552_
report_checks -through gcd_1/_carry_out_and_/B -fields input_pins

set db [ord::get_db]
if { $db eq "NULL" } {
  error "db is not defined"
}
set chip [$db getChip]
if { $chip eq "NULL" } {
  error "chip is not defined"
}
set block [$chip getBlock]
if { $block eq "NULL" } {
  error "block is not defined"
}
odb::dbDatabase_beginEco $block

set_debug_level ODB replace_design_check_sanity 1
puts "### module is swapped to Brent-Kung ###"
replace_hier_module gcd_1/_552_ LCU_16_BRENT_KUNG
report_cell_usage gcd_1/_552_
report_checks -through gcd_1/_carry_out_and_/B -fields input_pins

puts "### Undo module swap to get back Kogge-Stone ###"
odb::dbDatabase_endEco $block
odb::dbDatabase_undoEco $block
report_cell_usage gcd_1/_552_
report_checks -through gcd_1/_carry_out_and_/B -fields input_pins

run_equivalence_test replace_hier_mod5 \
  -lib_dir ./Nangate45/work_around_yosys/ \
  -remove_cells "None"
