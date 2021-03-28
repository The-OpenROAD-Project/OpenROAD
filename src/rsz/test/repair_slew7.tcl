# repair_design max_slew 2 corners
source "helpers.tcl"

define_corners fast slow
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef
read_def repair_slew5.def

set_wire_rc -layer metal3
estimate_parasitics -placement
# under the cap limit but high enough for slew violation
set_load 50 [get_net u2zn]

repair_design
