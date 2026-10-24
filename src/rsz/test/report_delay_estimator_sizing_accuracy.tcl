# Sizing delay-estimator report for comparison with Rebuffer-tree reports.
#
# This regression shows the standard sizing accuracy format:
#   fanin stage -> target sized cell -> fanout stage
# with estimated vs golden slew, delay, wire delay, and load cap.
#
# Sizing stage window checked by this report:
#   fanin stage                  target stage              fanout stage
#   -----------                  ------------              ------------
#   r1/Q
#     |
#     | r1q wire
#     v
#   u1/A      [u1: BUF_X1 -> BUF_X4] u1/Z
#                 ^ target cell      |
#                   sized in ECO     | u1z wire
#                   journal          v
#                                    u2/A                  u2/Z [BUF_X1]
#
# The report compares estimated and golden values for the three stages above:
# fanin driver r1/Q, target driver u1/Z, and fanout driver u2/Z.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup1.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

puts [join [list \
  "Sizing comparison: stage-window report compares estimated vs golden" \
  "after cell sizing."] " "]
report_delay_estimator_accuracy \
  -inst u1 \
  -lib_cell BUF_X4 \
  -estimator stage_window \
  -delay_levels 1
