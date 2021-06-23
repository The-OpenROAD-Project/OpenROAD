source "helpers.tcl"

set blif [rmp::create_blif "" "" "" ""]
set db [ord::get_db]
read_lef "./Nangate45/Nangate45.lef"
read_def "./design.def"
read_liberty "./Nangate45/Nangate45_typ.lib"

set chip [$db getChip]
set block [$chip getBlock]


rmp::blif_add_instance $blif "_g1_"
rmp::blif_add_instance $blif "_g2_"


rmp::blif_dump $blif "./results/out.blif"

set isDiff [diff_files "./results/out.blif" "test_blif_writer.blif.ok"]
if {$isDiff != 0} {
    exit 1
}

puts "pass"
exit
