read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def
read_sdc gcd.sdc

set_wire_rc -layer metal3
estimate_parasitics -placement

report_worst_slack

report_design_area

set tiehi "NangateOpenCellLibrary/LOGIC1_X1/Z"
set tielo "NangateOpenCellLibrary/LOGIC0_X1/Z"

restructure -target area -abc_logfile results/abc_rcon.log  -tielo_pin $tielo -tiehi_pin $tiehi

report_design_area

exit
