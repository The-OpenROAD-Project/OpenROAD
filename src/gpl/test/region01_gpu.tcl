# Multi-region (dual voltage domain) global placement on the GPU backend.
#
# Regression guard for the per-region density-field fix: gpl builds one
# NesterovBase per placement region but historically shared a single GPU
# DeviceState bin grid, so with >1 region the GPU density solve read another
# region's (or wrong-sized) bin buffers — crashing or silently corrupting the
# placement. With per-region RegionDensityField the GPU result must track the
# CPU result.
#
# This is the same dual-domain design as region01 but run on the GPU path
# (ENABLE_GPU=1). The golden integration tests force ENABLE_GPU=0; this test is
# registered only on ENABLE_GPU builds and left on the GPU path, then checks
# the total HPWL against the CPU reference within a tolerance (the GPU Poisson
# FFT diverges ~1e-4 from the CPU Ooura FFT, so an exact DEF diff is not used).
source helpers.tcl
set test_name region01
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130_fd_sc_hd__nom.tlef
read_lef sky130hd/sky130_fd_sc_hd.lef
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

# Total HPWL from instance/IO-pin bounding boxes (same computation as
# report_hpwl.tcl). Compared to the CPU reference within tolerance.
set db [::ord::get_db]
set block [[$db getChip] getBlock]
set total_hpwl 0
foreach net [$block getNets] {
  set xMin 1e30
  set yMin 1e30
  set xMax -1e30
  set yMax -1e30
  foreach iTerm [$net getITerms] {
    set instBox [[$iTerm getInst] getBBox]
    set xMin [expr min($xMin, [$instBox xMin])]
    set yMin [expr min($yMin, [$instBox yMin])]
    set xMax [expr max($xMax, [$instBox xMax])]
    set yMax [expr max($yMax, [$instBox yMax])]
  }
  foreach bTerm [$net getBTerms] {
    foreach bPin [$bTerm getBPins] {
      set pinBox [$bPin getBBox]
      set xMin [expr min($xMin, [$pinBox xMin])]
      set yMin [expr min($yMin, [$pinBox yMin])]
      set xMax [expr max($xMax, [$pinBox xMax])]
      set yMax [expr max($yMax, [$pinBox yMax])]
    }
  }
  if { $xMin != 1e30 && $yMin != 1e30 && $xMax != -1e30 && $yMax != -1e30 } {
    set total_hpwl [expr $total_hpwl + $xMax - $xMin + $yMax - $yMin]
  }
}
puts "region01_gpu total hpwl: $total_hpwl"

# CPU reference HPWL (measured with ENABLE_GPU=0 on the same design). The GPU
# multi-region placement must land within 3% of it. Measured: CPU 19.9M;
# fixed GPU 20.0M (+0.7%, the GPU/CPU FFT divergence); the pre-fix shared-bin
# bug gave 43.7M (+119%) because each region's solve clobbered the others'.
# 3% comfortably separates the two.
set ref_hpwl 19908543
set rel_err [expr abs($total_hpwl - $ref_hpwl) / double($ref_hpwl)]
puts "region01_gpu hpwl rel_err vs CPU reference: $rel_err"
check "region01_gpu multi-region HPWL within 3% of CPU reference" \
  {expr {$rel_err < 0.03}} 1
exit_summary
