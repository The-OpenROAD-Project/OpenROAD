source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_def scan_opt_sky130.def
set_dft_config \
  -max_chains 1 \
  -scan_enable_name_pattern sce \
  -scan_in_name_pattern sci \
  -scan_out_name_pattern sco
create_clock [get_ports clk] -name clk -period 130
execute_dft_plan
scan_opt

set def_file [make_result_file scan_opt_sky130.out.def]
write_def $def_file
diff_files $def_file scan_opt_sky130.defok
