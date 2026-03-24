# Test for RDL router keeping exising swires
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan_flipchip.def

set swire [odb::dbSWire_create [[ord::get_db_block] findNet VDD] FIXED]
odb::dbSBox_create $swire [[ord::get_db_tech] findLayer metal10] \
  570000 1602000 670000 1900000 IOWIRE

rdl_route -layer metal10 -width 4 -spacing 4 "VDD"

set def_file [make_result_file "rdl_route_keep_existing.def"]
write_def $def_file
diff_files $def_file "rdl_route_keep_existing.defok"
