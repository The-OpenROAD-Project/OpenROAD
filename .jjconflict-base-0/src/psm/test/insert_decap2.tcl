# insert_decap for sky130hd/gcd
source "helpers.tcl"

read_lef "sky130hd/sky130hd.tlef"
read_lef "sky130hd/sky130hd_std_cell.lef"

read_liberty sky130hd/sky130_fd_sc_hd__ss_n40C_1v40.lib

source sky130hd/sky130hd.rc

read_def "sky130hd_data/insert_decap_gcd.def"

analyze_power_grid -net {VDD}
insert_decap -target_cap 10.5 -cells {"sky130_fd_sc_hd__decap_3" 2.5}

check_placement

set def_file [make_result_file insert_decap2.def]
write_def $def_file
diff_file $def_file insert_decap2.defok
