# Inverting registers (asap7 DFFHQNx1 stores the complement of D) keep their
# cell across self-dual gates: r1 across the inverters and the buffer, and
# the three registers of cone 2 across the majority gate.  The result is
# proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
set libs [list \
  asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz \
  asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz \
  asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib]
foreach lib $libs {
  read_liberty $lib
}
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_verilog retime_dsec_inverting_asap7.v
link_design top
create_clock -period 60 clk

write_verilog_for_dsec retime_dsec_inverting_asap7 before
report_worst_slack -max -digits 2
retime
report_worst_slack -max -digits 2
run_dsec_test retime_dsec_inverting_asap7 -liberty_files $libs \
  -clock clk -resetn rst_n

report_new_cells
