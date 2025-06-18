# test the insertion of power switches into a design.
# The power switch control is connected in a STAR configuration
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130_spm/delayed_serial_adder.lef

read_def sky130_spm/floorplan.def

set_voltage_domain -name CORE -power VPWR -ground VGND

define_pdn_grid \
    -name stdcell_grid \
    -starts_with POWER \
    -voltage_domain CORE \
    -pins "met4 met5"

add_pdn_stripe \
    -grid stdcell_grid \
    -layer met4 \
    -width 2 \
    -pitch 15 \
    -offset 5 \
    -spacing 1.7 \
    -starts_with POWER -extend_to_core_ring

add_pdn_stripe \
    -grid stdcell_grid \
    -layer met5 \
    -width 2 \
    -pitch 15 \
    -offset 5 \
    -spacing 1.7 \
    -starts_with POWER -extend_to_core_ring

add_pdn_connect \
    -grid stdcell_grid \
    -layers "met4 met5"

add_pdn_stripe \
    -grid stdcell_grid \
    -layer met1 \
    -width 0.48 \
    -followpins

add_pdn_connect \
    -grid stdcell_grid \
    -layers "met1 met4"

define_pdn_grid \
    -macro \
    -default \
    -name macro \
    -starts_with POWER \
    -halo "10 10"

add_pdn_connect \
    -grid macro \
    -layers "met4 met5"

pdngen

set def_file [make_result_file sky130_spm_floating_bpin.def]
write_def $def_file
diff_files sky130_spm_floating_bpin.defok $def_file
