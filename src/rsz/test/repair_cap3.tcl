# repair_design -max_cap_margin
source "max_slew_cap.tcl"

repair_design -max_cap_margin 10

report_check_types -max_slew -max_cap -max_fanout
