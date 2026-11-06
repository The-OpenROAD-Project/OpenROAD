# LEGACY repair_setup on a critical-path slice of gf180 aes_cipher_top with 38
# violating endpoints. The golden log covers buffer removal, sizing up, pin
# swaps and buffering, and its worst path runs from a flip-flop to a flip-flop.

source "helpers.tcl"

read_liberty gf180/gf180mcu_fd_sc_mcu9t5v0__tt_025C_5v00.lib.gz
read_lef gf180/gf180mcu_5LM_1TM_9K_9t_tech.lef
read_lef gf180/gf180mcu_5LM_1TM_9K_9t_sc.lef
read_def repair_setup_path6.def
read_sdc repair_setup_path6.sdc

set ::env(METAL_OPTION) "5LM_1TM"
set ::env(CORNER) "TC"
source gf180/setRC.tcl
estimate_parasitics -placement

repair_timing -setup -phases "LEGACY"
estimate_parasitics -placement
report_checks -path_delay max -format full -digits 4
