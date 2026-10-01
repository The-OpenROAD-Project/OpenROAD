# BufferToInvertersMove coverage on a hierarchical netlist. Same long-wire geometry
# as repair_setup_buffer_to_inverters, but the BUF_X4 lives in submod m1 and its iterms
# carry modnets after link_design -hier. Before the fix, dbNetwork::net(pin)
# returned the modnet wrapper which sta::Sta::connectPin then reinterpret_cast
# to a dbNet, segfaulting inside dbITerm::connect. The fix makes
# BufferToInvertersCandidate cache flat dbNets and modnets separately.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog repair_setup_buffer_to_inverters_hier.v
link_design reg1 -hier
read_def -floorplan_initialize repair_setup_buffer_to_inverters_hier.def
create_clock -period 1.0 clk
set_load 0.0 [all_outputs]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
repair_timing -setup -sequence "buffer_to_inverters" -max_passes 20
report_checks -fields input -digits 3
