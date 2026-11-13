# Critical VT swap walks back from a violating clock gate enable.
# It must swap the enable logic, never the clock tree feeding the gate.
source "helpers.tcl"
foreach vt {RVT LVT SLVT} {
  read_liberty asap7/asap7sc7p5t_INVBUF_${vt}_FF_nldm_220122.lib.gz
  read_liberty asap7/asap7sc7p5t_SEQ_${vt}_FF_nldm_220123.lib
}
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_SL_1x_220121a.lef
read_verilog repair_setup_crit_vt_clock_gate.v
link_design top
create_clock -name clk -period 60 [get_ports clk]
set_propagated_clock [all_clocks]
set_input_delay 0 -clock clk [get_ports {in1 in2}]
source asap7/setRC.tcl

repair_timing -setup -sequence vt_swap -skip_last_gasp -max_iterations 20

proc master_of { inst } {
  return [[[[ord::get_db_block] findInst $inst] getMaster] getName]
}
check "clock buffer keeps its VT" { master_of clkbuf } BUFx2_ASAP7_75t_R
check "clock gate keeps its VT" { master_of icg } ICGx1_ASAP7_75t_R
check "enable logic is swapped" { master_of u8 } INVx1_ASAP7_75t_SL
exit_summary
