# repair_design 2 loads driven from middle
#
#         u4-out2
#            |
#            |1500u
#            |
# in1--u1--u2-
#            |
#            |1500u
#            |
#            u3-out1
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire4.def

set_wire_rc -layer metal3
estimate_parasitics -placement
# zero estimated parasitics to output ports
set_load 0 [get_net out1]

report_long_wires 4
repair_design -max_wire_length 900
report_long_wires 6
