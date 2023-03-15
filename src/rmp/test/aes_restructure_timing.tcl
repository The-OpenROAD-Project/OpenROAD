read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def aes_synthesis.def
read_sdc aes_nangate45.sdc

report_worst_slack
report_design_area

set tiehi "LOGIC1_X1/Z"
set tielo "LOGIC0_X1/Z"

set_thread_count [exec getconf _NPROCESSORS_ONLN]
restructure -liberty_file Nangate45/Nangate45_typ.lib  \
            -target timing \
            -depth_threshold 6 \
            -abc_logfile ./results/abc_rcon.log \
            -tielo_port $tielo \
            -tiehi_port $tiehi \
            -work_dir ./results
report_worst_slack
report_design_area
if {[sta::worst_slack -max] > 0} {
    puts "pass"
} else {
    puts "fail"
}
