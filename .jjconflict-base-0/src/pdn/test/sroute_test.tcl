# test for add_sroute_connect
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130_temp_sensor/srouteHEADER.lef
read_lef sky130_temp_sensor/srouteSLC.lef

read_def sky130_temp_sensor/4_cts.def

add_sroute_connect \
  -net "VIN" \
  -outerNet "VDD" \
  -layers {met1 met4} \
  -cut_pitch {200 200} \
  -fixed_vias {M3M4_PR_M} \
  -metalwidths {1000 1000} \
  -metalspaces {800} \
  -ongrid {met3 met4} \
  -insts "temp_analog_1.a_header_0 temp_analog_1.a_header_1 temp_analog_1.a_header_2"

set def_file [make_result_file sroute_test.def]
write_def $def_file

diff_files sroute_test.defok $def_file
