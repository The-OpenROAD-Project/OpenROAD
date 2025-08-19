source "helpers.tcl"
read_liberty ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib
read_liberty ihp-sg13g2/sg13g2_stdcell_typ_1p50V_25C.lib
read_lef ihp-sg13g2/sg13g2_tech.lef
read_lef ihp-sg13g2/sg13g2_stdcell.lef

read_verilog gcd_ihp.v
link gcd

report_opt_config

report_buffers -filtered
rsz::report_fast_buffer_sizes

set_opt_config -disable_buffer_pruning true
report_opt_config

report_buffers -filtered
rsz::report_fast_buffer_sizes
