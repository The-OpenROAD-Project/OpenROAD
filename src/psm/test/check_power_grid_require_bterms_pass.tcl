# Check that check_power_grid passes with terminals
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/aes_bterms.def
catch {check_power_grid -net VDD -dont_require_terminals} err
puts $err
