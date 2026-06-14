# InvBufferMove coverage: one BUF_X4 drives a 2.75 mm wire to a far flop.
# Repair_timing -setup -sequence "invbuffer" should pick this buffer and
# replace it with two cascaded inverters placed along the long wire,
# committing at least one InvBufferMove.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_inv_buffer.def
create_clock -period 1.0 clk
set_load 0.0 [all_outputs]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
repair_timing -setup -sequence "invbuffer" -max_passes 20
report_checks -fields input -digits 3
