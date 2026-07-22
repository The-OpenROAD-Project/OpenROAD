#
# replace_hier_mod6
#
# Used a new gcd design (synthesized by Yosys v0.58) for test.
# - Test if uninstantiated module can be swapped.
# - Test if four different ALUs (HC, BK, KS, SK) have equivalent functions.
# - Test if setup timing is the same after the swap (HC->BK->KS->SK->HC).
#
source "helpers.tcl"

set test_name replace_hier_mod6

read_lef "data/sky130hd/sky130_fd_sc_hd.tlef"
read_lef "data/sky130hd/sky130_fd_sc_hd_merged.lef"

read_liberty "sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib"
#set_debug_level ODB dbReadVerilog 1
#set_debug_level ODB replace_design 1
read_verilog $test_name.v
link_design -hier gcd
create_clock -name CLK -period 1 clk

# _001_ is a critical ALU output.
# _120_ is an ALU instance.
estimate_parasitics -placement
report_checks -fields input_pins -through _001_
report_cell_usage _120_

# For eqy, write a verilog before replace_hier_module
# - EQ check is disabled in the regression due ot its excessive runtime.
# - To enable the EQ check, you should set the envar "EQUIVALENCE_CHECK".
write_verilog_for_eqy $test_name before "None"

set_debug_level ODB replace_design_check_sanity 1
proc do_swap { from_alu to_alu } {
  variable test_name
  puts "\n============================================="
  puts "$from_alu -> $to_alu"
  puts "============================================="
  set result [catch { replace_hier_module _120_ $to_alu }]
  if { $result == 0 } {
    puts "Successfully replaced hier module"
    estimate_parasitics -placement
    report_checks -fields input_pins -through _001_
    report_cell_usage _120_
    run_equivalence_test $test_name \
      -lib_dir ./sky130hd/work_around_yosys/ \
      -remove_cells "None"
  } else {
    puts "Failed to replace hier module"
  }
}

# Swap ALU module sequence: HC (initial) -> BK -> KS -> SK -> HC
set alus [list \
  "ALU_16_HAN_CARLSON" \
  "ALU_16_BRENT_KUNG" \
  "ALU_16_KOGGE_STONE" \
  "ALU_16_SKLANSKY" \
  "ALU_16_HAN_CARLSON"]

for { set i 0 } { $i < [llength $alus] - 1 } { incr i } {
  do_swap [lindex $alus $i] [lindex $alus [expr { $i + 1 }]]
}
