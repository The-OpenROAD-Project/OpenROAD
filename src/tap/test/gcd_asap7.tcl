source "helpers.tcl"
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_def asap7_data/gcd.def

set def_file [make_result_file gcd_asap7.def]

tapcell \
  -distance 25 \
  -tapcell_master TAPCELL_ASAP7_75t_R \
  -endcap_master TAPCELL_ASAP7_75t_R

write_def $def_file

diff_file gcd_asap7.defok $def_file
