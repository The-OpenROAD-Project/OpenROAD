# repair_design long wire from core to pad
source "resizer_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty pad.lib
read_lef Nangate45/Nangate45.lef
read_lef pad.lef
read_def repair_wire11.def

initialize_floorplan -die_area "0 0 1200 1200" \
  -core_area "10 10 390 390" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

source Nangate45/Nangate45.vars
source $tracks_file

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
report_check_types -max_slew -max_cap -max_fanout -violators

repair_design -max_wire_length 600
report_check_types -max_slew -max_cap -max_fanout -violators
check_in_core
