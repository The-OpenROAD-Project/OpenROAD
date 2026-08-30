# A padring is routinely handed over PLACED rather than FIXED.
# CoreGrid::setupDirectConnect connects to a pad as soon as it is placed, but
# Grid::makeInitialObstructions only collected obstructions from instances that
# were fixed, so a PLACED padring produced connections and no obstructions at
# all - the grid ran straight through it, 17 shapes landing on pad metal.  A
# padframe cell now obstructs as soon as it has a location.
source "helpers.tcl"

read_lef ihp_ethmac/tech.lef
read_lef ihp_ethmac/sg13g2.lef
read_lef ihp_tensorcore/sg13g2_io.lef

read_def ihp_tensorcore/floorplan.def

foreach inst [[ord::get_db_block] getInsts] {
  if { [[$inst getMaster] isPad] } {
    $inst setPlacementStatus PLACED
  }
}

set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}

define_pdn_grid \
  -name stdcell_grid \
  -starts_with POWER \
  -voltage_domains {CORE}

add_pdn_stripe \
  -grid stdcell_grid \
  -layer TopMetal1 \
  -width 5 \
  -pitch 100 \
  -offset 50 \
  -starts_with POWER \
  -extend_to_boundary

add_pdn_stripe \
  -grid stdcell_grid \
  -layer TopMetal2 \
  -width 5 \
  -pitch 100 \
  -offset 50 \
  -starts_with POWER \
  -extend_to_boundary

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

set def_file [make_result_file pads_placed_not_fixed.def]
write_def $def_file
diff_files pads_placed_not_fixed.defok $def_file
