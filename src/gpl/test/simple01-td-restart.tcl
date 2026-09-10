# Regression for the FISTA momentum restart after a non-virtual timing-driven
# iteration (GPL-0111).
#
# In the ORFS nangate45/leon3_vta_bus design, the second non-virtual
# timing-driven iteration (overflow 0.19) let repair_design replace ~95k of
# ~270k movable GCells. The Nesterov momentum coefficient (~0.995) accumulated
# before the repair was kept, so the placer kept extrapolating the pre-repair
# trajectory on the changed objective: HPWL grew from 18.8M to 37.5M um while
# overflow kept falling below 0.10, and an SRAM output buffer ended 4.7 mm away
# from its driver (4.2 ns net delay). Restarting the momentum (curA = 1) right
# after the topology replacement keeps the post-repair HPWL bounded.
#
# A max_fanout of 2 makes the non-virtual repair_design rebuild most of this
# small netlist at both timing-driven iterations (overflow 0.63 and 0.19), so
# the log must show GPL-0111 after each of them and the placement must still
# converge to the golden result.
source helpers.tcl
set test_name simple01-td-restart
read_liberty ./library/nangate45/NangateOpenCellLibrary_typical.lib

read_lef ./nangate45.lef
read_def ./simple01-td.def

create_clock -name core_clock -period 2 clk

set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5

set_max_fanout 2 [current_design]

global_placement -timing_driven

# check reported wns
estimate_parasitics -placement
report_worst_slack

set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
