# test for connecting grid to pads on all edges
source "helpers.tcl"

read_lef gf180/gf180mcu_6LM_1TM_9K_7t_tech.lef
read_lef gf180/gf180mcu_fd_sc_mcu7t5v0.lef
read_lef gf180/gf180mcu_fd_io.lef

read_def gf180_data/floorplan.def

# core voltage domain
set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}

# stdcell grid
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
  -layer Metal1 \
  -width 0.6 \
  -followpins

add_pdn_stripe \
  -grid stdcell_grid \
  -layer Metal4 \
  -width 1.6 \
  -pitch 153.6 \
  -offset 16.32 \
  -spacing 1.7 \
  -starts_with POWER \
  -extend_to_core_ring

add_pdn_stripe \
  -grid stdcell_grid \
  -layer Metal5 \
  -width 1.6 \
  -pitch 153.18 \
  -offset 16.65 \
  -spacing 1.7 \
  -starts_with POWER \
  -extend_to_core_ring

add_pdn_connect -grid stdcell_grid -layers "Metal1 Metal4"
add_pdn_connect -grid stdcell_grid -layers "Metal2 Metal3"
add_pdn_connect -grid stdcell_grid -layers "Metal2 Metal4"
add_pdn_connect -grid stdcell_grid -layers "Metal2 Metal5"
add_pdn_connect -grid stdcell_grid -layers "Metal3 Metal4"
add_pdn_connect -grid stdcell_grid -layers "Metal4 Metal5"

pdngen

set def_file [make_result_file pads_connect_from_non_pref_edge.def]
write_def $def_file
diff_files pads_connect_from_non_pref_edge.defok $def_file
