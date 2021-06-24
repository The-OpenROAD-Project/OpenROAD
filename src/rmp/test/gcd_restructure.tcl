read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def
read_sdc gcd.sdc

set_wire_rc -layer metal3
estimate_parasitics -placement

report_worst_slack

report_design_area

restructure -target area -abc_logfile results/abc.log

report_design_area

exit
