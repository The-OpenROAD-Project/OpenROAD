source "helpers.tcl"
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_TT_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_LVT_TT_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_SLVT_TT_nldm_220122.lib.gz
read_lef asap7/asap7_tech_1x_201209.lef
#read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_SL_1x_220121a.lef

read_verilog gcd_asap7_no_lef.v
link gcd

report_buffers -filtered
rsz::report_fast_buffer_sizes

set_opt_config -disable_buffer_pruning true

report_buffers -filtered
rsz::report_fast_buffer_sizes

report_equiv_cells -vt BUFx2_ASAP7_75t_R
report_equiv_cells -vt BUFx2_ASAP7_75t_L
report_equiv_cells -vt BUFx2_ASAP7_75t_SL
