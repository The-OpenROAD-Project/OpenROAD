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


rmp::blif_dump $blif "./results/blif_writer_input_output.blif"

set isDiff [diff_files "./results/blif_writer_input_output.blif" "blif_writer_input_output.blif.ok"]
if {$isDiff != 0} {
    exit 1
}

puts "pass"
exit
