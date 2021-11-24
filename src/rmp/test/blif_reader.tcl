source "helpers.tcl"

set blif [rmp::create_blif "LOGIC1_X1" "Z" "LOGIC0_X1" "Z"]
set db [ord::get_db]
read_lef "./Nangate45/Nangate45.lef"
read_def "./design_in_out.def"
read_liberty "./Nangate45/Nangate45_typ.lib"

set chip [$db getChip]
set block [$chip getBlock]


rmp::blif_add_instance $blif "_i1_"
rmp::blif_add_instance $blif "_i2_"
rmp::blif_add_instance $blif "_i3_"
rmp::blif_add_instance $blif "_i4_"
rmp::blif_add_instance $blif "_i5_"
rmp::blif_add_instance $blif "_i6_"
rmp::blif_add_instance $blif "_i7_"
rmp::blif_add_instance $blif "_i8_"


rmp::blif_read $blif "./blif_reader.blif"

write_def "./results/blif_reader.def"
diff_files "./results/blif_reader.def" "blif_reader.def.ok"

