# Rebuffer-tree delay-estimator report with side-load impact.
#
# The report focuses on the r1/Q fanout cone in
# report_delay_estimator_rebuffer_side_paths.def.
#
# Pre-Rebuffer tree:
#   r1/Q
#     |-- u1/A -> u1 -> u2 -> u3 -> u4 -> u5 -> r2/D  (critical path)
#     |-- side_r3/A -> side_r3/Z -> r3/D
#     |-- side_r4/A -> side_r4/Z -> r4/D
#     |-- r5/D
#     |-- r6/D
#     |-- r7/D
#     |-- r8/D
#     |-- r9/D
#     |-- r10/D
#     |-- r11/D
#     `-- r12/D
#
# Post-Rebuffer tree estimated by this command:
#   r1/Q
#     |-- u1/A -> u1 -> u2 -> u3 -> u4 -> u5 -> r2/D  (target branch)
#     |-- r8/D, r9/D, r10/D, r12/D
#     `-- inserted-buffer subtree for side loads such as side_r3/A, side_r4/A,
#         and r5/D..r7/D
#
# The .ok file compares estimated vs golden timing for the target branch and
# the selected side-load paths.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def report_delay_estimator_rebuffer_side_paths.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

puts [join [list \
  "Critical-path comparison: Rebuffer tree impact summary compares" \
  "estimated vs golden arrival delta."] " "]
puts [join [list \
  "Side-load comparison: Rebuffer tree side-path impact summary compares" \
  "each side-load estimated delta vs golden delta."] " "]
report_delay_estimator_accuracy_for_rebuffering \
  -inst r1 \
  -driver_pin r1/Q
