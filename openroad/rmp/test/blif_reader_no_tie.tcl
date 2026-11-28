# Reading a blif constant with no tie cell available is an error.
source "helpers.tcl"

set blif [rmp::create_blif "" "" "" ""]
read_lef "./Nangate45/Nangate45.lef"
read_def "./design_in_out.def"
read_liberty "./Nangate45/Nangate45_typ.lib"

rmp::blif_add_instance $blif "_i1_"

catch { rmp::blif_read $blif "./blif_reader_no_tie.blif" } error
puts $error
