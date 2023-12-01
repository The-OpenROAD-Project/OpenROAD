# group1 should fit in the same section with group2
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def group_pins10.def

set group1 {bus[0] bus[1] bus[2] bus[3] bus[4] bus[5] bus[6] bus[7] bus[8] bus[9] bus[10] bus[11] bus[12] bus[13] bus[14] bus[15] bus[16] bus[17] bus[18] bus[19] bus[20] bus[21] bus[22] bus[23] bus[24] bus[25] bus[26] bus[27] bus[28] bus[29] bus[30] bus[31] bus[32] bus[33] bus[34] bus[35] bus[36] bus[37] bus[38] bus[39] bus[40] bus[41] bus[42] bus[43] bus[44] bus[45] bus[46] bus[47] bus[48] bus[49] bus[50] bus[51] bus[52] bus[53] bus[54] bus[55] bus[56] bus[57] bus[58] bus[59] bus[60] bus[61] bus[62] bus[63] bus[64] bus[65] bus[66] bus[67] bus[68] bus[69] bus[70] bus[71] bus[72] bus[73] bus[74] bus[75] bus[76] bus[77] bus[78] bus[79] bus[80] bus[81] bus[82] bus[83] bus[84] bus[85] bus[86] bus[87] bus[88] bus[89] bus[90] bus[91] bus[92] bus[93] bus[94] bus[95] bus[96] bus[97] bus[98] bus[99] bus[100] bus[101]}

set_io_pin_constraint -region left:* -pin_names $group1
set_io_pin_constraint -group -order -pin_names $group1
set_io_pin_constraint -region left:* -pin_names clk
set_io_pin_constraint -group -order -pin_names clk

place_pins -hor_layer metal3 \
           -ver_layer metal2

set def_file [make_result_file group_pins10.def]

write_def $def_file

diff_file group_pins10.defok $def_file
