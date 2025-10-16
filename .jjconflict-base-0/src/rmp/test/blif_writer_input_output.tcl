source "helpers.tcl"

set blif [rmp::create_blif "" "" "" ""]
set db [ord::get_db]
read_lef "./Nangate45/Nangate45.lef"
read_def "./design_in_out.def"
read_liberty "./Nangate45/Nangate45_typ.lib"

set chip [$db getChip]
set block [$chip getBlock]


rmp::blif_add_instance $blif "_i5_"
rmp::blif_add_instance $blif "_i6_"
rmp::blif_add_instance $blif "_i7_"


set result_blif [make_result_file "blif_writer_input_output.blif"]
rmp::blif_dump $blif "$result_blif"

diff_files "$result_blif" "blif_writer_input_output.blif.ok"
