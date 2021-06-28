# repair_design -max_slew_margin
source "max_slew_cap.tcl"

repair_design -max_slew_margin .5

report_check_types -max_slew -max_cap -max_fanout
