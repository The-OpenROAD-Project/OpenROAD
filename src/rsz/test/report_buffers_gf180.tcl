source "helpers.tcl"
read_liberty gf180/gf180mcu_fd_sc_mcu9t5v0__tt_025C_5v00.lib.gz
read_lef gf180/gf180mcu_5LM_1TM_9K_9t_tech.lef
read_lef gf180/gf180mcu_5LM_1TM_9K_9t_sc.lef

read_verilog jpeg.v
link_design jpeg_encoder -hier
read_sdc jpeg.sdc

report_buffers -filtered
rsz::report_fast_buffer_sizes

set_opt_config -disable_buffer_pruning true

report_buffers -filtered
rsz::report_fast_buffer_sizes
