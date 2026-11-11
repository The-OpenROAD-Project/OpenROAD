# pad_pin_edge with CUGR.
source "helpers.tcl"
read_lef "gf180/gf180mcu_6LM_1TM_9K_7t_tech.lef"
read_lef "gf180/gf180mcu_fd_sc_mcu7t5v0.lef"
read_lef "pad_pin_edge.lef"
read_def "pad_pin_edge.def"
make_tracks

set_routing_layers -signal Metal2-Metal5

set guide_file1 [make_result_file pad_pin_edge_cugr1.guide]
set guide_file2 [make_result_file pad_pin_edge_cugr2.guide]

global_route -use_cugr -verbose
write_guides $guide_file1

# DRT also places access points on the bond pad.
pin_access -verbose 0
global_route -use_cugr -verbose
write_guides $guide_file2

diff_files pad_pin_edge_cugr.guideok1 $guide_file1
diff_files pad_pin_edge_cugr.guideok2 $guide_file2
