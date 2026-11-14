# repair_design falls back to a faster VT when no size in the sizing set
# meets a clock -data_path slew limit.  With keep_sizing_vt and a 1.0 sizing
# area limit, DFFHQNx1_ASAP7_75t_R has no other size; its QN slew into eight
# INVx4 loads exceeds the 120 ps clock limit, while the same footprint in LVT
# meets it, so repairDriverSlew swaps the driver to the LVT footprint (it only
# tried the sizing set before the fix).  repair_design may also buffer the net.
source "helpers.tcl"

read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_LVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_SEQ_LVT_FF_nldm_220123.lib
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_def repair_slew_clk_data_path_vt.def

create_clock -name clk -period 500 [get_ports clk]
set_max_transition 120 -data_path [get_clocks clk]
set_opt_config -keep_sizing_vt true -sizing_area_limit 1.0

source asap7/setRC.tcl
estimate_parasitics -placement

proc master { inst_name } {
  return [[[[ord::get_db_block] findInst $inst_name] getMaster] getName]
}

report_check_types -max_slew -violators -digits 1
check "drvr/QN violates the clock data-path limit" \
  { expr { [get_property [get_pins drvr/QN] slew_max] > 120 } } 1

set insts_before [llength [get_cells *]]
repair_design
report_check_types -max_slew -violators -digits 1

check "drvr is swapped to the LVT footprint" { master drvr } \
  DFFHQNx1_ASAP7_75t_L
check "drvr/QN meets the clock data-path limit after the VT swap" \
  { expr { [get_property [get_pins drvr/QN] slew_max] <= 120 } } 1
check "no max slew violations remain" { sta::max_slew_violation_count } 0

exit_summary
