# repair_power binding: every option reaches the command, a slow cell goes to
# a higher VT, and a tolerance outside 0-100 is an error.  The behavior is
# tested in cpp/TestRepairPower.cc.
source "helpers.tcl"
source asap7/asap7.vars
foreach vt {RVT LVT SLVT} {
  read_liberty asap7/asap7sc7p5t_INVBUF_${vt}_FF_nldm_220122.lib.gz
  read_liberty asap7/asap7sc7p5t_SIMPLE_${vt}_FF_nldm_211120.lib.gz
  read_liberty asap7/asap7sc7p5t_AO_${vt}_FF_nldm_211120.lib.gz
  read_liberty asap7/asap7sc7p5t_OA_${vt}_FF_nldm_211120.lib.gz
  read_liberty asap7/asap7sc7p5t_SEQ_${vt}_FF_nldm_220123.lib
}
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_SL_1x_220121a.lef
read_def gcd_asap7_placed.def
read_sdc gcd.sdc
create_clock -name core_clock -period 400 [get_ports clk]
source asap7/setRC.tcl

foreach inst [get_cells *] {
  set ref [get_property $inst ref_name]
  if {
    [regsub {_ASAP7_75t_R$} $ref {_ASAP7_75t_SL} fast]
    && [get_lib_cells -quiet $fast] != ""
  } {
    replace_cell $inst [get_lib_cells $fast]
  }
}
estimate_parasitics -placement

proc slvt_count { } {
  return [llength [get_cells -quiet -filter "ref_name =~ *_ASAP7_75t_SL"]]
}
set slvt [slvt_count]
repair_power -setup_margin 1 -hold_margin 1 -setup_tolerance 1 -verbose
check "cells go to a higher VT" { expr { [slvt_count] < $slvt } } 1
check_error "-setup_tolerance above 100" \
  { repair_power -setup_tolerance 150 } STA-0576
exit_summary
