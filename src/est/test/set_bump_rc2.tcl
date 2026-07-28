# set_bump_rc values are used for the port-to-bump net of a chip bump instance
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg3.def

create_clock -period 10 clk
set_input_delay -clock clk 0 in1
set_input_delay -clock clk 0 in2

# make r1 a chip bump, so the two-pin in1 -> r1 net is a pad net that takes
# the lumped bump RC
set block [ord::get_db_block]
set r1 [$block findInst r1]
set chip [[ord::get_db] getChip]
set region [odb::dbChipRegion_create $chip "f2f" $odb::dbChipRegion_Side_FRONT \
  "NULL"]
odb::dbChipBump_create $region $r1

set_bump_rc -resistance 2 -capacitance 100

# same wire RC values as make_parasitics1
set lambda .12
# kohm/square.
set m1_res_sq .08e-3
# ff/micron^2
set m1_area_cap 39e-3
# ff/micron.
set m1_edge_cap 57e-3
# 4 lambda wide wire
set wire_cap [expr { $m1_area_cap * $lambda * 4 + $m1_edge_cap * 2 }]
set wire_res [expr { $m1_res_sq / ($lambda * 4) }]
set_wire_rc -resistance $wire_res -capacitance $wire_cap
estimate_parasitics -placement

# in1 -> r1 carries the lumped bump RC
report_checks -from in1 -to r1 -fields {capacitance slew} -format full
# in2 -> r2 has no chip bump and keeps the estimated wire parasitics
report_checks -from in2 -to r2 -fields {capacitance slew} -format full
