# Compare full timing-update runtime on the placed GF180 JPEG design.
set test_name dcalc_full_update_runtime_gf180_jpeg
source "helpers.tcl"
source "dcalc_full_update_runtime_helpers.tcl"

read_liberty ../../rsz/test/gf180/gf180mcu_fd_sc_mcu9t5v0__tt_025C_5v00.lib.gz
read_lef ../../rsz/test/gf180/gf180mcu_5LM_1TM_9K_9t_tech.lef
read_lef ../../rsz/test/gf180/gf180mcu_5LM_1TM_9K_9t_sc.lef
read_def ../../tap/test/gf180/jpeg.def
read_sdc ../../rsz/test/jpeg.sdc
set ::env(METAL_OPTION) 5
set ::env(CORNER) TT
source ../../rsz/test/gf180/setRC.tcl
estimate_parasitics -placement

run_dcalc_full_update_runtime
