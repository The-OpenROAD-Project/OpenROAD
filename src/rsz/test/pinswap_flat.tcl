# repair_timing -setup combinational path
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

read_verilog pinswap_flat.v
link_design td1

read_sdc repair_setup2.sdc

#place the design
initialize_floorplan -die_area "0 0 40 1200"   -core_area "0 0 40 1200" -site FreePDK45_38x28_10R_NP_162NW_34O
global_placement -skip_nesterov_place
detailed_placement

source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal3
set_wire_rc -clock  -layer metal5
estimate_parasitics -placement

report_worst_slack
write_verilog_for_eqy repair_setup2 before "None"
repair_design
write_verilog before_ps.v
set_debug_level RSZ  "repair_setup" 3
repair_timing -setup -verbose
write_verilog pinswap_flat_out.v

