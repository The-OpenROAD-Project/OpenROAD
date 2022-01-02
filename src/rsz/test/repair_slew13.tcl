# repair_design impossibly small liberty max_transition
# in1--u1---------u2-out1
read_liberty repair_slew13.lib
read_lef sky130hd/sky130hd.tlef
read_lef repair_slew13.lef
read_def repair_slew13.def

set_wire_rc -layer met3
estimate_parasitics -placement

report_check_types -max_slew
repair_design
report_check_types -max_slew
