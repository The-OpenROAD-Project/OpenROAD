# insert_decap for sky130hd/gcd
source "helpers.tcl"

read_liberty sky130hd/sky130_fd_sc_hd__ss_n40C_1v40.lib
read_lef "sky130hd/sky130hd.tlef"
read_lef "sky130hd/sky130hd_std_cell.lef"
read_def "sky130hd_data/insert_decap_gcd.def"

source sky130hd/sky130hd.rc

analyze_power_grid -net {VDD}
insert_decap -target_cap 1000.5 -cells {"sky130_fd_sc_hd__decap_3" 0.93 "sky130_fd_sc_hd__decap_4" 0.124 "sky130_fd_sc_hd__decap_6" 0.186 "sky130_fd_sc_hd__decap_8" 0.248 "sky130_fd_sc_hd__decap_12" 0.362}

check_placement

set def_file [make_result_file insert_decap1.def]
write_def $def_file
diff_file $def_file insert_decap1.defok
