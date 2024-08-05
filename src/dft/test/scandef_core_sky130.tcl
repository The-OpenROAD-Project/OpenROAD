source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef register_bank_cores.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog scan_inserted_register_bank.v
link_design top -hier

read_def -incremental scandef_core_sky130.scandef

set def_file [make_result_file scandef_core_sky130.out.def]
write_def $def_file
diff_files $def_file scandef_core_sky130.defok
