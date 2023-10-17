# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -region bottom:* -group -order -pin_names {req_msg[0] req_msg[1] req_msg[2] req_msg[3] req_msg[4] req_msg[5] req_msg[6] req_msg[7] req_msg[8] req_msg[9] req_msg[10] req_msg[11] req_msg[12] req_msg[13] req_msg[14] req_msg[15] req_msg[16] req_msg[17] req_msg[18] req_msg[19] req_msg[20] req_msg[21] req_msg[22] req_msg[23] req_msg[24] req_msg[25] req_msg[26] req_msg[27] req_msg[28] req_msg[29] req_msg[30] req_msg[31]}
set_io_pin_constraint -mirrored_pins {req_msg[0] req_val}
ppl::set_slots_per_section 50
place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.36

set def_file [make_result_file add_constraint15.def]

write_def $def_file

diff_file add_constraint15.defok $def_file