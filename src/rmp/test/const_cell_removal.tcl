read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def rcon.def
read_sdc rcon.sdc

report_design_area

set tiehi "LOGIC1_X1/Z"
set tielo "LOGIC0_X1/Z"

restructure -liberty_file Nangate45/Nangate45_typ.lib -target area -abc_logfile results/abc_rcon.log  -tielo_port $tielo -tiehi_port $tiehi

report_design_area

exit
