source helpers.tcl

read_lef ./asap7/asap7_tech_1x_201209.lef
read_lef ./SingleBit/asap7sc7p5t_28_L_1x_220121a.lef
read_lib ./SingleBit/asap7sc7p5t_SEQ_LVT_TT_nldm_220123.lib
read_lef ./2BitTrayH2/asap7sc7p5t_DFFHQNV2X.lef
read_lib ./2BitTrayH2/asap7sc7p5t_DFFHQNV2X_LVT_TT_nldm_FAKE.lib
read_lef ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X.lef
read_lib ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X_LVT_TT_nldm_FAKE.lib

read_verilog ./mbff_hier.v
link_design -hier mbff_hier

set block [ord::get_db_block]
set i 0
foreach inst [$block getInsts] {
  set x [expr 2000 + ($i % 4) * 2000]
  set y [expr 2000 + ($i / 4) * 2000]
  $inst setLocation $x $y
  $inst setPlacementStatus PLACED
  incr i
}

create_clock -name clk -period 1000 [get_ports clk1]

cluster_flops -tray_weight 40.0 -timing_weight 0.0 \
  -max_split_size -1 -num_paths 0

set_wire_rc -layer M2
estimate_parasitics -placement
puts ESTIMATE_PARASITICS_OK
