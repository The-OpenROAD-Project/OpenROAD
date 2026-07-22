# Test hier module swap and reverse swap

source "helpers.tcl"
source Nangate45/Nangate45.vars
define_corners slow
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_lef Nangate45/Nangate45.lef

read_verilog replace_hier_mod1.v
link_design top -hier
create_clock -period 0.3 clk

#################################################
# No crash cases
#################################################

# 1. OK
#replace_hier_module bc1 inv_chain
#replace_hier_module bc1 buffer_chain
#exit

# 2. OK
#report_checks
#replace_hier_module bc1 inv_chain
#report_checks
#replace_hier_module bc1 buffer_chain
#exit

# 3. OK
#replace_hier_module bc1 inv_chain
#report_checks -through u1z -through r2/D -digits 3
#replace_hier_module bc1 buffer_chain
#exit

# 4. OK
#replace_hier_module bc1 inv_chain
#replace_hier_module bc1 buffer_chain
#report_checks -through u1z -through r2/D -digits 3
#replace_hier_module bc1 inv_chain
#exit

# 5. OK
#replace_hier_module bc1 inv_chain
#replace_hier_module bc1 buffer_chain
#replace_hier_module bc1 inv_chain
#replace_hier_module bc1 buffer_chain
#report_checks -through u1z -through r2/D -digits 3
#exit

# 6. OK
#report_checks
#replace_hier_module bc1 inv_chain
#replace_hier_module bc1 buffer_chain
#exit

# 7. OK
report_checks
set_debug_level ODB replace_design_check_sanity 1
replace_hier_module bc1 inv_chain
report_checks -through u1z
replace_hier_module bc1 buffer_chain
report_checks -through u1z
#exit


#################################################
# Crash cases
#################################################

# PROBLEM:
# - Swap module twice after "report_checks -through" caused crash.
# - Using "report_checks" w/ module swap is fine (no crash).
# - "report_checks -through" might create an internal cache.

# 1. Crash
#report_checks -through u1z -through r2/D -digits 3
#replace_hier_module bc1 inv_chain
#report_checks -through u1z -through r2/D -digits 3
#replace_hier_module bc1 buffer_chain
#report_checks -through u1z -through r2/D -digits 3
#exit

# 2. Crash
#report_checks -through u1z
#replace_hier_module bc1 inv_chain
#report_checks -through r2/D
#replace_hier_module bc1 buffer_chain
#report_checks -through r2/D
#exit
