source helpers.tcl
set test_name region01
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130_fd_sc_hd__nom.tlef
read_lef sky130hd/sky130_fd_sc_hd.lef
# read_lef sky130_ef_sc_hd.lef
read_verilog $test_name-spm.nl.v
read_verilog ./$test_name-dual_spm.v
link_design dual_spm
read_upf -file ./$test_name-dual_spm.upf

set_domain_area spm_inst_0_domain -area {40 40 250 150}
# must not overlap the y axis
set_domain_area spm_inst_1_domain -area {40 210 250 320}

initialize_floorplan \
  -utilization 5 \
  -core_space 2 \
  -site unithd

make_tracks li1 -x_offset 0.23 -x_pitch 0.46 -y_offset 0.17 -y_pitch 0.34
make_tracks met1 -x_offset 0.17 -x_pitch 0.34 -y_offset 0.17 -y_pitch 0.34
make_tracks met2 -x_offset 0.23 -x_pitch 0.46 -y_offset 0.23 -y_pitch 0.46
make_tracks met3 -x_offset 0.34 -x_pitch 0.68 -y_offset 0.34 -y_pitch 0.68
make_tracks met4 -x_offset 0.46 -x_pitch 0.92 -y_offset 0.46 -y_pitch 0.92
make_tracks met5 -x_offset 1.70 -x_pitch 3.40 -y_offset 1.70 -y_pitch 3.40


set spm_inst_0_power_net "VDD_0"
set gnd_net "GND"
add_global_connection \
  -net $spm_inst_0_power_net \
  -inst_pattern {spm_inst_0.*} \
  -pin_pattern {VPWR} \
  -power
add_global_connection \
  -net $gnd_net \
  -inst_pattern {spm_inst_0.*} \
  -pin_pattern {VGND} \
  -ground
set_voltage_domain \
  -region spm_inst_0_domain \
  -power $spm_inst_0_power_net \
  -ground $gnd_net
define_pdn_grid \
  -name spm_inst_0_grid \
  -pins met4 \
  -voltage_domain spm_inst_0_domain
add_pdn_stripe \
  -grid spm_inst_0_grid \
  -layer met1 \
  -width 0.48 \
  -followpins
add_pdn_stripe \
  -grid spm_inst_0_grid \
  -layer met4 \
  -width 2 \
  -spacing 2 \
  -pitch 40
add_pdn_connect \
  -grid spm_inst_0_grid \
  -layers {met1 met4}
set spm_inst_1_power_net VDD_1
add_global_connection \
  -net $spm_inst_1_power_net \
  -inst_pattern {spm_inst_1.*} \
  -pin_pattern {VPWR} \
  -power
add_global_connection \
  -net $gnd_net \
  -inst_pattern {spm_inst_1.*} \
  -pin_pattern {VGND} \
  -ground
set_voltage_domain \
  -region spm_inst_1_domain \
  -power $spm_inst_1_power_net \
  -ground $gnd_net
define_pdn_grid \
  -name spm_inst_1_grid \
  -pins met4 \
  -voltage_domain spm_inst_1_domain
add_pdn_stripe \
  -grid spm_inst_1_grid \
  -layer met1 \
  -width 0.48 \
  -followpins
add_pdn_stripe \
  -grid spm_inst_1_grid \
  -layer met4 \
  -width 2 \
  -spacing 2 \
  -offset 20 \
  -pitch 40
add_pdn_connect \
  -grid spm_inst_1_grid \
  -layers {met1 met4}
pdngen

cut_rows \
  -endcap_master sky130_fd_sc_hd__decap_3
place_pins \
  -hor_layers met3 \
  -ver_layers met2


global_placement -density 0.7
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
