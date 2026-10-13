# Reduced version of the design from
# https://github.com/The-OpenROAD-Project/OpenROAD/issues/9994:
# a sg13g2 pad ring around an empty core, with the core straps and the core
# ring sharing TopMetal1/TopMetal2. The core straps block the pad to core ring
# connections, and the connections that survive are heavily biased towards
# one net (VDD gets several per side, VSS only one).
source "helpers.tcl"

read_lef ihp_ethmac/tech.lef
read_lef ihp_ethmac/sg13g2.lef
read_lef ihp_tensorcore/sg13g2_io.lef

read_def ihp_tensorcore/floorplan.def

set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}

define_pdn_grid \
  -name stdcell_grid \
  -starts_with POWER \
  -voltage_domains {CORE}

add_pdn_stripe \
  -grid stdcell_grid \
  -layer TopMetal1 \
  -width 2.2 \
  -pitch 75.6 \
  -offset 13.6 \
  -spacing 4 \
  -starts_with POWER \
  -extend_to_core_ring

add_pdn_stripe \
  -grid stdcell_grid \
  -layer TopMetal2 \
  -width 2.2 \
  -pitch 75.6 \
  -offset 13.6 \
  -spacing 4 \
  -starts_with POWER \
  -extend_to_core_ring

add_pdn_connect -grid stdcell_grid -layers "TopMetal1 TopMetal2"

add_pdn_ring \
  -grid stdcell_grid \
  -layers "TopMetal1 TopMetal2" \
  -widths "15 15" \
  -spacings "5 5" \
  -core_offsets "4.5 4.5" \
  -allow_out_of_die \
  -connect_to_pads

pdngen

set def_file [make_result_file pads_ihp_sg13g2_balance.def]
write_def $def_file
diff_files pads_ihp_sg13g2_balance.defok $def_file
