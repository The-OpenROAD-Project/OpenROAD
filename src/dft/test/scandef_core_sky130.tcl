source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef register_bank_cores.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog scan_inserted_register_bank.v
link_design top -hier

read_def -incremental scandef_core_sky130.scandef
write_def scandef_core_sky130.out.def
diff_files scandef_core_sky130.out.def scandef_core_sky130.defok
