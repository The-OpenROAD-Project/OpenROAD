# repair_design max_slew liberty pin max_transition
source "helpers.tcl"
source "hi_fanout.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
# DFF_MAX_SLEW/Q max_transition
read_liberty repair_slew2.lib
read_lef repair_slew2.lef

set def_file [make_result_file "repair_slew2.def"]
write_hi_fanout_def1 $def_file 20 \
  "rdrv" "DFF_MAX_SLEW" "CK" "Q" \
  "r" "DFF_X1" "CK" "D" \
  "metal1"

read_def $def_file
create_clock -period 1 clk1

set_wire_rc -layer metal3
estimate_parasitics -placement

report_check_types -max_slew -violators
repair_design
report_check_types -max_slew -violators
