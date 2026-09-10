# Multi-region GPU global placement with DIFFERENT-SIZE voltage domains.
#
# Companion to region01_gpu.tcl. That test uses two same-size domains, which
# exercises the cross-region field interleaving but produces equal bin counts.
# Here the two domains have different areas (see set_domain_area below), so the
# per-region bin grids differ in size — this is the case where the pre-fix
# shared DeviceState bin grid was not just stale but the WRONG SIZE: an earlier
# region's solve/gather indexed a buffer sized for a different region, an
# out-of-bounds device access (crash in release / the bin-count check now in
# NesterovDeviceContext::densitySolveIteration). It also exercises the
# per-region geometry (binCntX/Y, gridLx/Ly) plumbing, which same-size domains
# cannot distinguish. With per-region RegionDensityField the GPU result tracks
# the CPU result.
#
# Run on the GPU path (ENABLE_GPU=1, pinned in CMake); checks total HPWL
# against the CPU reference within tolerance (the GPU Poisson FFT diverges
# ~1e-4 from the CPU Ooura FFT, so an exact DEF diff is not used).
source helpers.tcl
set test_name region01
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130_fd_sc_hd__nom.tlef
read_lef sky130hd/sky130_fd_sc_hd.lef
read_verilog $test_name-spm.nl.v
read_verilog ./$test_name-dual_spm.v
link_design dual_spm
read_upf -file ./$test_name-dual_spm.upf

set_domain_area spm_inst_0_domain -area {40 40 250 160}
# must not overlap the y axis
set_domain_area spm_inst_1_domain -area {40 220 190 320}

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
puts "region01_gpu_asym total hpwl: $total_hpwl"

# CPU reference HPWL (measured with ENABLE_GPU=0 on this different-size-domain
# design). The GPU multi-region placement must land within 3% of it. Measured:
# CPU 18.75M; fixed GPU 18.80M (+0.28%, the GPU/CPU FFT divergence). Pre-fix,
# the differing per-region bin counts made this an out-of-bounds device access.
# 3% comfortably covers the FFT divergence while catching gross corruption.
set ref_hpwl 18746115
set rel_err [expr abs($total_hpwl - $ref_hpwl) / double($ref_hpwl)]
puts "region01_gpu_asym hpwl rel_err vs CPU reference: $rel_err"
check "region01_gpu_asym multi-region HPWL within 3% of CPU reference" \
  {expr {$rel_err < 0.03}} 1
exit_summary
