# repair_antennas using gcd_grt.db (global_route already executed)
#source "helpers.tcl"
#set_thread_count max
#read_liberty "sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib"
#read_db "sky130hd/jpeg_grt.db"

#check_antennas
#repair_antennas
#check_antennas
#check_placement

#set guide_file [make_result_file repair_antennas_from_odb.guide]
#write_guides $guide_file
#diff_file repair_antennas_from_odb.guideok $guide_file

#set def_file [make_result_file repair_antennas_from_odb.def]
#write_def $def_file
#diff_file repair_antennas_from_odb.defok $def_file

# repair_antennas. def file generated using the openroad-flow
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_db "sky130hs/gcd_grt.db"

check_antennas
repair_antennas
check_antennas
check_placement

set guide_file [make_result_file repair_antennas_from_odb.guide]
write_guides $guide_file
diff_file repair_antennas_from_odb.guideok $guide_file

set def_file [make_result_file repair_antennas_from_odb.def]
write_def $def_file
diff_file repair_antennas_from_odb.defok $def_file

