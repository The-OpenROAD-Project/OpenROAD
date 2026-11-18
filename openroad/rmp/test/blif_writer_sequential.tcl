source "helpers.tcl"

set blif [rmp::create_blif "" "" "" ""]
set db [ord::get_db]
read_lef "./Nangate45/Nangate45.lef"
read_def "./design_sequential.def"
read_liberty "./Nangate45/Nangate45_typ.lib"

set chip [$db getChip]
set block [$chip getBlock]


rmp::blif_add_instance $blif "_i1_"
rmp::blif_add_instance $blif "_i2_"
rmp::blif_add_instance $blif "_i3_"
rmp::blif_add_instance $blif "_r1_"


set result_blif [make_result_file "blif_writer_sequential.blif"]
rmp::blif_dump $blif $result_blif

write_def [make_result_file "design_seq.def"]
diff_files $result_blif "blif_writer_sequential.blif.ok"
