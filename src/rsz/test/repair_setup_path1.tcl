# Critical-path slice extracted from gf180/riscv32i post-CTS ODB.
# Drives LEGACY repair_setup on a real RISC-V ALU sum critical path.

source "helpers.tcl"

read_liberty gf180/gf180mcu_fd_sc_mcu9t5v0__tt_025C_5v00.lib.gz
read_lef gf180/gf180mcu_5LM_1TM_9K_9t_tech.lef
read_lef gf180/gf180mcu_5LM_1TM_9K_9t_sc.lef
read_def repair_setup_path1.def
read_sdc repair_setup_path1.sdc

set ::env(METAL_OPTION) "5LM_1TM"
set ::env(CORNER) "TC"
source gf180/setRC.tcl
estimate_parasitics -placement

repair_timing -setup -phases "LEGACY"
estimate_parasitics -placement
report_checks -path_delay max -format full -digits 4
