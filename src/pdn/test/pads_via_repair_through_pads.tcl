# Regression for the problem behind
# https://github.com/The-OpenROAD-Project/OpenROAD/pull/10222: Grid::repairVias
# extended grid shapes through the padframe.
#
# A pad connection to an east pad runs horizontally even though Metal2 is a
# vertical layer, so it is parallel to the Metal3 stripe below.  The stripe is
# placed on the same track as the VDD connection of gf180mcu_fd_io__dvdd_east,
# the two overlap on the core ring, and a Metal2/Metal3 via is made there.  Via
# repair then called Shape::extendTo on both shapes and grew each one to cover
# the other: the Metal3 stripe was pulled east into the pad, and the Metal2 pad
# connection was pulled 3122 um west across the whole core.
#
# Neither extension was rejected because the pad had no obstructions at all -
# PdnGen::buildGrids removed every directly connected pad from the obstruction
# set.  With the pad's obstructions restored the Metal3 extension is rejected on
# the spot, no via is repaired, and the Metal2 connection is never dragged
# anywhere; the longest Metal2 shape in the golden is a 74 um pad connection.
source "helpers.tcl"

read_lef gf180/gf180mcu_6LM_1TM_9K_7t_tech.lef
read_lef gf180/gf180mcu_fd_sc_mcu7t5v0.lef
read_lef gf180_data/gf180mcu_fd_io.lef

read_def gf180_data/floorplan.def

set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}

define_pdn_grid \
  -name stdcell_grid \
  -starts_with POWER \
  -voltage_domains {CORE}

add_pdn_ring \
  -grid stdcell_grid \
  -layers "Metal4 Metal3" \
  -widths "15 15" \
  -spacings "1.7 1.7" \
  -core_offsets "6 6" \
  -connect_to_pads

add_pdn_stripe \
  -grid stdcell_grid \
  -layer Metal4 \
  -width 1.6 \
  -pitch 153.6 \
  -offset 16.32 \
  -spacing 1.7 \
  -starts_with POWER \
  -extend_to_core_ring

# offset picked so the stripe lands on the VDD connection of the east pad
add_pdn_stripe \
  -grid stdcell_grid \
  -layer Metal3 \
  -width 0.6 \
  -pitch 500 \
  -offset 81.24 \
  -number_of_straps 1 \
  -starts_with POWER \
  -extend_to_core_ring

add_pdn_connect -grid stdcell_grid -layers "Metal2 Metal3"
add_pdn_connect -grid stdcell_grid -layers "Metal2 Metal4"
add_pdn_connect -grid stdcell_grid -layers "Metal3 Metal4"

pdngen

set def_file [make_result_file pads_via_repair_through_pads.def]
write_def $def_file
diff_files pads_via_repair_through_pads.defok $def_file
