# check if global route works for single row designs
source "helpers.tcl"
read_liberty "sky130hs/sky130_fd_sc_hs__tt_025C_1v80.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def single_row.def

set guide_file [make_result_file single_row.guide]

set_routing_layers -signal li1-met1

global_route -verbose

write_guides $guide_file

diff_file single_row.guideok $guide_file
