# PARALLEL_EVAL is followed by CAPTURE_VT_SWAP, which swaps a capture-only
# register to the VT with the shorter setup time.  cap's only failing path
# ends at its D pin and its QN is unloaded, so no PARALLEL_EVAL target covers
# it and the run used to end without touching it.  In
# the asap7 FF libraries DFFHQNx1 LVT has a *longer* setup time than RVT and
# SLVT a shorter one, so the swap must pick the VT by setup margin, not by
# drive strength.
source "helpers.tcl"

read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_LVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_SLVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_SEQ_LVT_FF_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_SEQ_SLVT_FF_nldm_220123.lib
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_SL_1x_220121a.lef
read_def repair_setup_crit_vt_capture.def

create_clock -name clk -period 100 [get_ports clk]
set_input_delay 95 -clock clk [get_ports d_in]
set_input_transition 20 [get_ports d_in]

source asap7/setRC.tcl
estimate_parasitics -placement

proc master { inst_name } {
  return [[[[ord::get_db_block] findInst $inst_name] getMaster] getName]
}
proc d_slack { } {
  return [get_property [get_pins cap/D] slack_max]
}

set slack_before [d_slack]
check "cap/D violates setup" { expr { $slack_before < 0 } } 1

repair_timing -setup -phases PARALLEL_EVAL

check "the capture register is swapped to the shorter-setup VT" \
  { master cap } DFFHQNx1_ASAP7_75t_SL
check "its setup slack improves" { expr { [d_slack] > $slack_before } } 1

exit_summary
