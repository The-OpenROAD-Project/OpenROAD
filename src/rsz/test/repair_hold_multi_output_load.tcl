# Targeted regression for issue #10213. A tiny ASAP7 design contains one
# FAx1_ASAP7_75t_R whose CON output drives the repaired endpoint and whose
# SN output is on a different net. The launch net is marked dont_touch so
# repair_timing -hold must attempt repair on the FA CON net itself.
source "helpers.tcl"
source asap7/asap7.vars

read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef

read_verilog repair_hold_multi_output_load.v
link_design repair_hold_multi_output_load

initialize_floorplan \
  -site $site \
  -die_area {0 0 20 4} \
  -core_area {0 0 20 4}

place_inst -name u_launch -location {1 0.5}
place_inst -name u_fa -location {4 0.5}
place_inst -name u_capture_con -location {8 0.5}
place_inst -name u_capture_sn -location {12 0.5}
detailed_placement

create_clock -name clk -period 10 [get_ports clk]
set_min_delay 80 -from [get_clocks clk] -to [get_clocks clk]

source asap7/setRC.tcl
estimate_parasitics -placement

set_dont_touch launch_qn

report_checks -path_delay min -to [get_pins u_capture_con/D]
report_checks -path_delay min -to [get_pins u_capture_sn/D]

repair_timing -hold -allow_setup_violations

report_checks -path_delay min -to [get_pins u_capture_con/D]
