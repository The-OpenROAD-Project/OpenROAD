read_liberty Nangate45/Nangate45_typ.lib
# read_lef Nangate45/Nangate45.lef
# read_def aes_placed.def
read_db aes_nangate45_detailed_place-tcl.db
read_sdc aes_nangate45.sdc

#MIA set_layer_rc -via -resistance

set_wire_rc -signal -layer "metal3"
set_wire_rc -clock  -layer "metal6"

estimate_parasitics -placement
report_worst_slack -max -digits 3
# write_def aes_placed.def
# exit
# report_design_area

set tiehi "LOGIC1_X1/Z"
set tielo "LOGIC0_X1/Z"

set_thread_count 16
restructure -liberty_file Nangate45/Nangate45_typ.lib  \
            -target timing \
            -depth_threshold 6 \
            -abc_logfile ./results/abc_rcon.log \
            -tielo_port $tielo \
            -tiehi_port $tiehi \
            -work_dir ./results \
            -post_abc_script aes_post_abc.tcl
report_worst_slack
# report_design_area
# if {[sta::worst_slack -max] > 0} {
#     puts "pass"
# } else {
#     puts "fail"
# }
