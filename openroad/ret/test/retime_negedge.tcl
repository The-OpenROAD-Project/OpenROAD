# A negative-edge register is left in place with a warning while the
# positive-edge registers retime: r1 and r2 merge forward across the NAND,
# and rn stays, with g11, the gate it feeds, still reading its output.  Once
# rn is swapped to the positive-edge DFFHQx4 it moves: the second retime
# slides it across g11 and g12, so g11 reads g10, the gate in front of rn,
# and g13 reads the new register.
source "helpers.tcl"
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
read_verilog retime_negedge.v
link_design top
create_clock -period 80 clk

report_worst_slack -max -digits 2
retime
report_worst_slack -max -digits 2
report_kept {r1 r2 rn}
report_reads {g2/A g11/A g13/A}

replace_cell rn DFFHQx4_ASAP7_75t_R
retime
report_kept {r1 r2 rn}
report_reads {g2/A g11/A g13/A}
