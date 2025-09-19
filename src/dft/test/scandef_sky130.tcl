source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog scan_inserted_design_sky130.v
link_design scan_inserted_design

read_def -incremental scan_inserted_design_sky130.scandef
set result_def [make_result_file scandef_sky130.def]
write_def $result_def
diff_files $result_def "scandef_sky130.defok"
