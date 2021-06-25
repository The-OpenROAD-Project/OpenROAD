# repair_design weak driver for hi fanout
# 1 min size buffer meets slew/cap limits
source "helpers.tcl"
source "hi_fanout.tcl"

read_liberty sky130hd/sky130hd_tt.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef

set def_file [make_result_file "repair_slew6.def"]
write_hi_fanout_def1 $def_file 50 \
  "rdrv" "sky130_fd_sc_hd__o21ai_0" "" "Y" \
  "r" "sky130_fd_sc_hd__dfxtp_1" "" "D" 5000 \
  "met1" 1000

read_def $def_file

set_wire_rc -layer met3
estimate_parasitics -placement

report_check_types -max_slew -violators
repair_design
report_check_types -max_slew -violators
