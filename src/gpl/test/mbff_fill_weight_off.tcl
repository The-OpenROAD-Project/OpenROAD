# Baseline for the empty-slot (fill) penalty.
#   1. -fill_weight must be finite and non-negative (GPL 0124).
#   2. 7 flops with -clock_power_weight 3 bank into two 4-bit trays, one of
#      them partially filled (3 of 4 slots), when -fill_weight is 0.
# Pairs with mbff_fill_weight_on.tcl (same design/weights, -fill_weight 10).
source helpers.tcl

read_lef ./asap7/asap7_tech_1x_201209.lef
read_lef ./SingleBit/asap7sc7p5t_28_L_1x_220121a.lef
read_liberty ./SingleBit/asap7sc7p5t_SEQ_LVT_TT_nldm_220123.lib
read_lef ./2BitTrayH2/asap7sc7p5t_DFFHQNV2X.lef
read_liberty ./2BitTrayH2/asap7sc7p5t_DFFHQNV2X_LVT_TT_nldm_FAKE.lib
read_lef ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X.lef
read_liberty ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X_LVT_TT_nldm_FAKE.lib

read_verilog ./mbff_hier.v
link_design -hier mbff_hier

set block [ord::get_db_block]
set dbu [$block getDbUnitsPerMicron]
set i 0
foreach inst [$block getInsts] {
  set x [expr 2 * $dbu + ($i % 4) * 2 * $dbu]
  set y [expr 2 * $dbu + ($i / 4) * 2 * $dbu]
  $inst setLocation $x $y
  $inst setPlacementStatus PLACED
  incr i
}

# Drop one of the 8 flops so the count is not a multiple of the tray size.
odb::dbInst_destroy [$block findInst u_mid/l1/ff3]

create_clock -name clk -period 1000 [get_ports clk1]

# Validation: bad weights are rejected before any clustering happens, so the
# run below still sees the pristine single-bit netlist.
catch { cluster_flops -fill_weight -1.0 } msg
puts $msg
catch { cluster_flops -fill_weight inf } msg
puts $msg

cluster_flops -tray_weight 8.0 -timing_weight 0.0 -max_split_size -1 \
  -num_paths 0 -clock_power_weight 3.0 -fill_weight 0.0
