# repair_design slow driver into wire < max_length -> slew violation
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_slew3.def

set_wire_rc -layer metal3
estimate_parasitics -placement

# set max slew for violations
set_max_transition .03 [current_design]
report_check_types -max_slew -max_cap -max_fanout -violators
repair_design
report_check_types -max_slew -max_cap -max_fanout -violators
