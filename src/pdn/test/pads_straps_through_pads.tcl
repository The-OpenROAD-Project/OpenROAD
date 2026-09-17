# Regression for stripes that ran through the pad cells.
#
# PdnGen::buildGrids used to collect every pad that gets a direct connection
# into "insts_in_grids" and hand that set to Grid::makeInitialObstructions as
# the skip list, so the LEF obstructions of the connected pads never became PDN
# obstructions.  Nothing stopped the stripes at the padframe and they ran over
# the pads all the way to the die edge: 18 shapes covering 15490 um2 of pad
# cell, 17 of them over metal the pad declares.  A padframe cell is now
# exempted only from the grid that republishes its own obstructions, which a
# core grid does not do.
#
# The TopMetal1 and TopMetal2 stripes in the golden therefore stop at the
# padframe instead of reaching the die edge at 0 and 1200.  They do still enter
# the core facing 47.5 um of each pad, because sg13g2's TopMetal2 obstruction
# stops at local y 132.5 of a 180 um deep cell and declares that strip empty.
# Closing that needs a keep-out over the pad footprint, which cannot be turned
# on unconditionally because the bump flows route over their pads on purpose.
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

set def_file [make_result_file pads_straps_through_pads.def]
write_def $def_file
diff_files pads_straps_through_pads.defok $def_file
