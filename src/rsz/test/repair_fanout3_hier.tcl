# repair_design max_fanout
source "helpers.tcl"
source Nangate45/Nangate45.vars
source "hi_fanout.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog repair_fanout3_hier.v

link_design hi_fanout -hier


initialize_floorplan -die_area "0 0 40 1200" -core_area "0 0 40 1200" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
source $tracks_file
place_pins -hor_layers $io_placer_hor_layer \
  -ver_layers $io_placer_ver_layer
global_placement -skip_nesterov_place
detailed_placement

create_clock -period 10 clk1
set_max_fanout 10 [current_design]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

report_check_types -max_fanout

repair_design
report_check_types -max_fanout

sta::check_axioms

set verilog_file [make_result_file repair_fanout3_hier_out.v]
write_verilog $verilog_file
diff_files $verilog_file repair_fanout3_hier_out.vok
