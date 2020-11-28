# repair_design missing wire rc
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire1.def

set_layer_rc -layer metal3 -resistance 0.0 -capacitance 0.0
set_wire_rc -layer metal3
estimate_parasitics -placement

report_long_wires 2

repair_design -max_wire_length 600
